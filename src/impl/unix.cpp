/**
 * @file unix.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Definitions for unix implementation.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <glob.h>
#include <iostream>
#include <set>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "unix.hpp"

namespace spp
{

void SerialPort::SerialPortImpl::configurePort_()
{
    if (fdSerial_ < 0) {
        SPP_THROW(
            SerialPortException,
            "File descriptor invalid when calling configurePort_(), this might be a programming error!"
        );
    }

    struct termios opts;
    // POSIX requires a compulsory call to tcgetattr() first.
    if (tcgetattr(fdSerial_, &opts) == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "tcgetattr() call error when calling configurePort_().");
    }

    // Raw, no echo, binary.
    opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    opts.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    opts.c_oflag &= ~(ONLCR | OCRNL | OPOST);
    opts.c_lflag &= ~(ECHO | ECHOE | ECHONL | ICANON | ISIG | IEXTEN);

    if (parity_ == Parity::none) {
        opts.c_cflag &= ~PARENB;
    }
    else if (parity_ == Parity::even) {
        opts.c_cflag |= PARENB;
        opts.c_cflag &= ~PARODD;
    }
    else if (parity_ == Parity::odd) {
        opts.c_cflag |= (PARENB | PARODD);
    }
    else if (parity_ == Parity::space) {
        opts.c_cflag |= (PARENB | CMSPAR);
        opts.c_cflag &= ~PARODD;
    }
    else if (parity_ == Parity::mark) {
        opts.c_cflag |= (PARENB | CMSPAR | PARODD);
    }

    if (stopBits_ == StopBits::stop1) {
        opts.c_cflag &= ~CSTOPB;
    }
    else if (stopBits_ == StopBits::stop2) {
        opts.c_cflag |= CSTOPB;
    }

    opts.c_cflag &= ~CSIZE;
    if (dataBits_ == DataBits::data5) {
        opts.c_cflag |= CS5; // 5 bits per byte
    }
    else if (dataBits_ == DataBits::data6) {
        opts.c_cflag |= CS6; // 5 bits per byte
    }
    else if (dataBits_ == DataBits::data7) {
        opts.c_cflag |= CS7; // 5 bits per byte
    }
    else if (dataBits_ == DataBits::data8) {
        opts.c_cflag |= CS8; // 5 bits per byte
    }

    if (flowCtrl_ == FlowControl::none) {
        opts.c_cflag &= ~CRTSCTS;
        opts.c_iflag &= ~(IXON | IXOFF | IXANY);
    }
    else if (flowCtrl_ == FlowControl::software) {
        opts.c_cflag &= ~CRTSCTS;
        opts.c_iflag |= (IXON | IXOFF | IXANY);
    }
    else if (flowCtrl_ == FlowControl::hardware) {
        opts.c_cflag |= CRTSCTS;
        opts.c_iflag &= ~(IXON | IXOFF | IXANY);
    }

    // We use epoll to check for read/write events.
    opts.c_cc[VTIME] = 0;
    opts.c_cc[VMIN] = 0;

    speed_t baud = 0;
    if (baudrate_ == Baudrate::br1200) {
        baud = B1200;
    }
    else if (baudrate_ == Baudrate::br4800) {
        baud = B4800;
    }
    else if (baudrate_ == Baudrate::br9600) {
        baud = B9600;
    }
    else if (baudrate_ == Baudrate::br19200) {
        baud = B19200;
    }
    else if (baudrate_ == Baudrate::br38400) {
        baud = B38400;
    }
    else if (baudrate_ == Baudrate::br57600) {
        baud = B57600;
    }
    else if (baudrate_ == Baudrate::br115200) {
        baud = B115200;
    }
    else
    {
        SPP_THROW(SerialPortException, "Unknown baudrate is used.");
    }
    if (cfsetispeed(&opts, baud) == -1) {
        SPP_THROW_ERRNO(SerialPortException, errno, "cfsetispeed() call error, this could be a programming error!");
    }
    if (cfsetospeed(&opts, baud) == -1) {
        SPP_THROW_ERRNO(SerialPortException, errno, "cfsetospeed() call error, this could be a programming error!");
    }

    if (tcsetattr(fdSerial_, TCSANOW, &opts) == -1) {
        int err = errno;
        if (err == EINTR) {
            configurePort_();
        }
        else {
            SPP_THROW_ERRNO(PortIOException, errno, "tcsetattr() call error when calling configurePort_().");
        }
    }
}

int SerialPort::SerialPortImpl::closeSerial_()
{
    int err = 0;
    if (fdSerial_ != -1) {
        err = ::close(fdSerial_);
        fdSerial_ = -1;
    }
    return err;
}

int SerialPort::SerialPortImpl::closeEpollRead_()
{
    int err = 0;
    if (epollRead_ != -1) {
        err = ::close(epollRead_);
        epollRead_ = -1;
    }
    return err;
}

int SerialPort::SerialPortImpl::closeEpollWrite_()
{
    int err = 0;
    if (epollWrite_ != -1) {
        err = ::close(epollWrite_);
        epollWrite_ = -1;
    }
    return err;
}

void SerialPort::SerialPortImpl::closeEpoll_()
{
    closeEpollRead_();
    closeEpollWrite_();
}

std::vector<SerialPortInfo> SerialPort::SerialPortImpl::getDeviceInfo(const std::vector<std::string>& paths)
{
    using namespace std::filesystem;
    std::vector<SerialPortInfo> ports;
    std::set<std::string> current;
    for (const auto& str : paths) {
        path devicePath(str);
        path name = devicePath.stem();
        path realName = name;
        path realPath = devicePath;
        path symlinkPath = devicePath;
        bool isSymlink = false;

        if (realName == "ttyS0") {
            realName = name;
        }

        // Check for duplicates.
        if (current.find(name) != current.end()) {
            continue;
        } else {
            current.insert(name);
        }

        // If it is a symlink, we can only get information from the target path.
        if (is_symlink(devicePath)) {
            realPath = canonical(devicePath);
            realName = realPath.stem();
            isSymlink = true;
        }

        path sysDevicePath = "/sys/class/tty" / realName / "device";
        if (!exists(sysDevicePath)) {
            continue;
        }
        if (sysDevicePath.string().find("ttyUSB") != std::string::npos) {
            sysDevicePath = canonical(sysDevicePath).parent_path().parent_path();
        } else if (sysDevicePath.string().find("ttyACM0") != std::string::npos) {
            sysDevicePath = canonical(sysDevicePath).parent_path();
        } else {
            sysDevicePath = canonical(sysDevicePath) / "id";
        }

        auto getTextFromFile = [&](path p) -> std::string {
            std::ifstream ifs(p, std::ifstream::in);
            std::string str;
            std::getline(ifs, str);
            return str;
        };
        auto devnum = getTextFromFile(sysDevicePath / "devnum");
        auto manufacturer = getTextFromFile(sysDevicePath / "manufacturer");
        auto product = getTextFromFile(sysDevicePath / "product");
        auto serial = getTextFromFile(sysDevicePath / "serial");
        auto vid = getTextFromFile(sysDevicePath / "idVendor");
        auto pid = getTextFromFile(sysDevicePath / "idProduct");
        std::string hw;
        hw = "{idVendor}=={" + vid + "}" + " {idProduct}=={" + pid + "}" + " {serial}=={" + serial + "}";

        SerialPortInfo info;
        info.name = name;
        info.realName = realName;
        info.realPath = realPath;
        info.symlinkPath = symlinkPath;
        info.description = devnum + " " + manufacturer + " " + product + " " + serial;
        info.hardwareID = hw;
        ports.push_back(info);
    }
    return ports;
}

std::vector<SerialPortInfo> SerialPort::SerialPortImpl::listPorts() noexcept
{
    std::vector<std::string> patterns = {
        "/dev/*",
        "/dev/ttyACM*",
        "/dev/ttyS*",
        "/dev/ttyUSB*",
        "/dev/tty.*",
        "/dev/cu.*",
        "/dev/rfcomm*"
    };
    glob_t globRes;
    int ret = 0;
    bool first = true;
    for (const auto& str : patterns) {
        if (first) {
            first = false;
            ret = glob(str.c_str(), 0, NULL, &globRes);
        } else {
            ret = glob(str.c_str(), GLOB_APPEND, NULL, &globRes);
        }
    }
    std::vector<std::string> found;
    for (size_t i = 0; i < globRes.gl_pathc; i++) {
        found.push_back(globRes.gl_pathv[i]);
    }
    globfree(&globRes);
    return getDeviceInfo(found);
}

SerialPort::SerialPortImpl::SerialPortImpl() {}

SerialPort::SerialPortImpl::~SerialPortImpl()
{
    try {
        close();
    }
    catch(const PortIOException& e) {
        std::cerr << e.what() << std::endl;
    }
}

void SerialPort::SerialPortImpl::setPort(const std::string& port)
{
    port_ = port;
    if (isOpen_) {
        configurePort_();
    }
}

void SerialPort::SerialPortImpl::setParity(Parity parity)
{
    parity_ = parity;
    if (isOpen_) {
        configurePort_();
    }
}

void SerialPort::SerialPortImpl::setStopBits(StopBits stopBits)
{
    stopBits_ = stopBits;
    if (isOpen_) {
        configurePort_();
    }
}

void SerialPort::SerialPortImpl::setDataBits(DataBits dataBits)
{
    dataBits_ = dataBits;
    if (isOpen_) {
        configurePort_();
    }
}

void SerialPort::SerialPortImpl::setFlowCtrl(FlowControl flowCtrl)
{
    flowCtrl_ = flowCtrl;
    if (isOpen_) {
        configurePort_();
    }
}

void SerialPort::SerialPortImpl::setBaudrate(Baudrate baudrate)
{
    baudrate_ = baudrate;
    if (isOpen_) {
        configurePort_();
    }
}

const std::string& SerialPort::SerialPortImpl::getPort() const noexcept
{
    return port_;
}

Parity SerialPort::SerialPortImpl::getParity() const noexcept
{
    return parity_;
}

StopBits SerialPort::SerialPortImpl::getStopBits() const noexcept
{
    return stopBits_;
}

DataBits SerialPort::SerialPortImpl::getDataBits() const noexcept
{
    return dataBits_;
}

FlowControl SerialPort::SerialPortImpl::getFlowCtrl() const noexcept
{
    return flowCtrl_;
}

Baudrate SerialPort::SerialPortImpl::getBaudrate() const noexcept
{
    return baudrate_;
}

void SerialPort::SerialPortImpl::open()
{
    if (isOpen_) {
        return;
    }

    if (port_.empty()) {
        SPP_THROW(SerialPortException, "Serial port device path cannot be empty.");
    }

    fdSerial_ = ::open(port_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fdSerial_ == -1) {
        int err = errno;
        switch (err) {
            case EINTR:
                open();
                return;
            default:
                SPP_THROW_ERRNO(PortIOException, err, "open() syscall error when calling open().");
        }
    }

    epollRead_ = epoll_create1(0);
    if (epollRead_ == -1) {
        closeSerial_();
        SPP_THROW_ERRNO(PortIOException, errno, "epoll_create() call error when calling open() to create read epoll.");
    }
    epollWrite_ = epoll_create1(0);
    if (epollWrite_ == -1) {
        closeEpollRead_();
        closeSerial_();
        SPP_THROW_ERRNO(PortIOException, errno, "epoll_create() call error when calling open() to create write epoll.");
    }

    struct epoll_event evt;
    evt.events = EPOLLIN;
    evt.data.fd = fdSerial_;
    if (epoll_ctl(epollRead_, EPOLL_CTL_ADD, fdSerial_, &evt) == -1) {
        closeSerial_();
        closeEpoll_();
        SPP_THROW_ERRNO(PortIOException, errno, "epoll_ctl() call error when calling open() to control read epoll.");
    }
    evt.events = EPOLLOUT;
    if (epoll_ctl(epollWrite_, EPOLL_CTL_ADD, fdSerial_, &evt) == -1) {
        closeSerial_();
        closeEpoll_();
        SPP_THROW_ERRNO(PortIOException, errno, "epoll_ctl() call error when calling open() to control write epoll.");
    }

    configurePort_();
    isOpen_ = true;
}

bool SerialPort::SerialPortImpl::isOpen() const noexcept
{
    return isOpen_;
}

void SerialPort::SerialPortImpl::close()
{
    if (!isOpen_) {
        return;
    }
    /**
     * After a call to close() for epoll and serial port file descriptors (fd), in most cases, the fd are considered to
     * be released. So, we consider both are 'closed' even in the event they return errors.
     * We just throw an exception after setting open state.
     */
    int errR = closeEpollRead_();
    int errW = closeEpollWrite_();
    int errS = closeSerial_();
    isOpen_ = false;
    if (errR != 0) {
        SPP_THROW_ERRNO(PortIOException, errR, "Error when attempting to close read epoll instance.");
    }
    if (errW != 0) {
        SPP_THROW_ERRNO(PortIOException, errW, "Error when attempting to close write epoll instance.");
    }
    if (errS != 0) {
        SPP_THROW_ERRNO(PortIOException, errS, "Error when attempting to close serial port.");
    }
}

int SerialPort::SerialPortImpl::getHandle() const noexcept
{
    return fdSerial_;
}

bool SerialPort::SerialPortImpl::waitReadable(uint32_t timeoutMs)
{
    bool ready = false;
    if (epollRead_ == -1) {
        return ready;
    }

    struct epoll_event evt;
    int count = epoll_wait(epollRead_, &evt, 1, timeoutMs);
    if (count == -1) {
        int err = errno;
        if (err != EINTR) {
            SPP_THROW_ERRNO(PortIOException, err, "epoll_wait() call error when caling waitReadable().");
        }
    }
    else if (count > 0) {
        if (evt.events & EPOLLIN) {
            ready = true;
        }
    }

    return ready;
}

bool SerialPort::SerialPortImpl::waitWritable(uint32_t timeoutMs)
{
    bool ready = false;
    if (epollWrite_ == -1) {
        return ready;
    }

    struct epoll_event evt;
    int count = epoll_wait(epollWrite_, &evt, 1, timeoutMs);
    if (count == -1) {
        int err = errno;
        if (err != EINTR) {
            SPP_THROW_ERRNO(PortIOException, err, "epoll_wait() call error when caling waitWritable().");
        }
    }
    else if (count > 0) {
        if (evt.events & EPOLLOUT) {
            ready = true;
        }
    }

    return ready;
}

size_t SerialPort::SerialPortImpl::available() const
{
    if (!isOpen_) {
        return 0;
    }
    int count = 0;
    if (ioctl(fdSerial_, FIONREAD, &count) == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling available().");
    }
    return static_cast<size_t>(count);
}

size_t SerialPort::SerialPortImpl::read(uint8_t* buffer, size_t size, uint32_t timeoutMs, uint32_t timeoutMultiplierMs)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to read when serial port is closed.");
    }

    size_t bytesRead = 0;
    uint8_t* buf = buffer;

    // Even if timeoutMs == 0, we grab whatever is available first.
    ssize_t n = ::read(fdSerial_, buf, size);
    if (n > 0) {
        bytesRead = n;
        buf += n;
    }

    size_t remaining = size - bytesRead;
    uint32_t totalTimeout = timeoutMs + (timeoutMultiplierMs * remaining);
    MsTimer timer(totalTimeout);
    while (bytesRead < size && !timer.hasElapsed()) {
        if (!waitReadable(timer.remaining().count())) {
            continue;
        }

        remaining = size - bytesRead;
        ssize_t n = ::read(fdSerial_, buf, remaining);
        if (n == -1) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            else {
                SPP_THROW_ERRNO(PortIOException, err, "Error reading data from serial port.");
            }
        }
        else if (n == 0) {
            SPP_THROW(
                PortIOException,
                "Serial port reports it is ready to read but return no data. Is device still connected?"
            );
        }
        bytesRead += n;
        buf += n;
    }

    return bytesRead;
}

size_t SerialPort::SerialPortImpl::write(const uint8_t* data, size_t size, uint32_t timeoutMs, uint32_t timeoutMultiplierMs)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to write when serial port is closed.");
    }

    size_t bytesWrite = 0;
    const uint8_t* buf = data;

    ssize_t n = ::write(fdSerial_, buf, size);
    if (n > 0) {
        bytesWrite += n;
        buf += n;
    }

    size_t remaining = size - bytesWrite;
    uint32_t totalTimeout = timeoutMs + (timeoutMultiplierMs * remaining);
    MsTimer timer(totalTimeout);
    while (bytesWrite < size && !timer.hasElapsed()) {
        if (!waitWritable(timer.remaining().count())) {
            continue;
        }

        remaining = size - bytesWrite;
        ssize_t n = ::write(fdSerial_, buf, remaining);
        if (n == -1) {
            int err = errno;
            if (err == EINTR) {
                continue;
            }
            else {
                SPP_THROW_ERRNO(PortIOException, err, "Error writing data to serial port.");
            }
        }
        bytesWrite += n;
        buf += n;
    }

    return bytesWrite;
}

void SerialPort::SerialPortImpl::drain()
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call drain() when serial port is closed.");
    }
    int ret = tcdrain(fdSerial_);
    if (ret == -1) {
        int err = errno;
        if (err == EINTR) {
            drain();
            return;
        }
        SPP_THROW_ERRNO(PortIOException, err, "tcdrain() call error when calling drain().");
    }
}

void SerialPort::SerialPortImpl::flush()
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call flush() when serial port is closed.");
    }
    int ret = tcflush(fdSerial_, TCIOFLUSH);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "tcflush() call error when calling flush().");
    }
}

void SerialPort::SerialPortImpl::flushInput()
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call flushInput() when serial port is closed.");
    }
    int ret = tcflush(fdSerial_, TCIFLUSH);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "tcflush() call error when calling flushInput().");
    }
}

void SerialPort::SerialPortImpl::flushOutput()
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call flushOutput() when serial port is closed.");
    }
    int ret = tcflush(fdSerial_, TCOFLUSH);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "tcflush() call error when calling flushOutput().");
    }
}

void SerialPort::SerialPortImpl::sendBreak(int duration)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call sendBreak() when serial port is closed.");
    }
    int ret = tcsendbreak(fdSerial_, duration);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "tcsendbreak() call error when calling sendBreak().");
    }
}

void SerialPort::SerialPortImpl::setBreak(bool level)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call setBreak() when serial port is closed.");
    }
    if (level) {
        int ret = ioctl(fdSerial_, TIOCSBRK);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setBreak(true).");
        }
    } else {
        int ret = ioctl(fdSerial_, TIOCCBRK);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setBreak(false).");
        }
    }
}

bool SerialPort::SerialPortImpl::waitForChange()
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call waitForChange() when serial port is closed.");
    }
    int evt = (TIOCM_CTS | TIOCM_DSR | TIOCM_RI | TIOCM_CD);
    int ret = ioctl(fdSerial_, TIOCMIWAIT, &evt);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling waitForChange().");
    }
    return true;
}

void SerialPort::SerialPortImpl::setRTS(bool level)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call setRTS() when serial port is closed.");
    }
    int cmd = TIOCM_RTS;
    if (level) {
        int ret = ioctl(fdSerial_, TIOCMBIS, &cmd);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setRTS(true).");
        }
    } else {
        int ret = ioctl(fdSerial_, TIOCMBIC, &cmd);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setRTS(false).");
        }
    }
}

void SerialPort::SerialPortImpl::setDTR(bool level)
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call setDTR() when serial port is closed.");
    }
    int cmd = TIOCM_DTR;
    if (level) {
        int ret = ioctl(fdSerial_, TIOCMBIS, &cmd);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setDTR(true).");
        }
    } else {
        int ret = ioctl(fdSerial_, TIOCMBIC, &cmd);
        if (ret == -1) {
            SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling setDTR(false).");
        }
    }
}

bool SerialPort::SerialPortImpl::getCTS() const
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call getCTS() when serial port is closed.");
    }
    int stat = 0;
    int ret = ioctl(fdSerial_, TIOCMGET, &stat);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling getCTS().");
    }
    return stat & TIOCM_CTS;
}

bool SerialPort::SerialPortImpl::getDSR() const
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call getDSR() when serial port is closed.");
    }
    int stat = 0;
    int ret = ioctl(fdSerial_, TIOCMGET, &stat);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling getDSR().");
    }
    return stat & TIOCM_DSR;
}

bool SerialPort::SerialPortImpl::getRI() const
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call getRI() when serial port is closed.");
    }
    int stat = 0;
    int ret = ioctl(fdSerial_, TIOCMGET, &stat);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling getRI().");
    }
    return stat & TIOCM_RI;
}

bool SerialPort::SerialPortImpl::getCD() const
{
    if (!isOpen_) {
        SPP_THROW(PortClosedException, "Not allowed to call getCD() when serial port is closed.");
    }
    int stat = 0;
    int ret = ioctl(fdSerial_, TIOCMGET, &stat);
    if (ret == -1) {
        SPP_THROW_ERRNO(PortIOException, errno, "ioctl() call error when calling getCD().");
    }
    return stat & TIOCM_CD;
}

} // namespace spp
