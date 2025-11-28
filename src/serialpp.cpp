/**
 * @file serialpp.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Contains the definitions for serialpp.hpp.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#include "serialpp.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#if defined(unix) || defined(__unix__) || defined(__unix)
#include "impl/unix.hpp"
#endif

namespace spp {

SerialPortException::SerialPortException(const std::string& prefix) noexcept
{
    std::stringstream ss;
    ss << "[" << prefix << "]";
    desc_ = ss.str();
}

SerialPortException::SerialPortException(const std::string& prefix, const std::string& file, int line, int err, const std::string& message) noexcept
{
    std::stringstream ss;
    ss << "[" << prefix << "]: " << message << " [Errno " << err << "]: " << std::strerror(err) << " [File]: '" << file << ":" << line << "'";
    desc_ = ss.str();
}

SerialPortException::SerialPortException(const std::string& prefix, const std::string& file, int line, const std::string& message) noexcept
{
    std::stringstream ss;
    ss << "[" << prefix << "]: " << message << " [File]: '" << file << ":" << line << "'";
    desc_ = ss.str();
}

SerialPortException::SerialPortException() noexcept {}

SerialPortException::SerialPortException(const std::string& file, int line, int err, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, err, message)
{
}

SerialPortException::SerialPortException(const std::string& file, int line, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, message)
{
}

SerialPortException::SerialPortException(const SerialPortException& other) noexcept
{
    desc_ = other.desc_;
}

SerialPortException& SerialPortException::operator=(const SerialPortException& other) noexcept
{
    desc_ = other.desc_;
    return *this;
}

const char* SerialPortException::what() const noexcept
{
    return desc_.c_str();
}

PortClosedException::PortClosedException() noexcept : SerialPortException(TYPE_NAME_) {}

PortClosedException::PortClosedException(const std::string& file, int line, int err, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, err, message)
{
}

PortClosedException::PortClosedException(const std::string& file, int line, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, message)
{
}

PortClosedException::PortClosedException(const PortClosedException& other) noexcept
{
    desc_ = other.desc_;
}

PortClosedException& PortClosedException::operator=(const PortClosedException& other) noexcept
{
    desc_ = other.desc_;
    return *this;
}

PortIOException::PortIOException() noexcept : SerialPortException(TYPE_NAME_) {}

PortIOException::PortIOException(const std::string& file, int line, int err, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, err, message)
{
}

PortIOException::PortIOException(const std::string& file, int line, const std::string& message) noexcept :
    SerialPortException(TYPE_NAME_, file, line, message)
{
}

PortIOException::PortIOException(const PortIOException& other) noexcept
{
    desc_ = other.desc_;
}

PortIOException& PortIOException::operator=(const PortIOException& other) noexcept
{
    desc_ = other.desc_;
    return *this;
}

MsTimer::MsTimer(std::chrono::milliseconds periodMs) : start_(std::chrono::steady_clock::now()), period_(periodMs) {}

MsTimer::MsTimer(uint32_t periodMs) : MsTimer(std::chrono::milliseconds(periodMs)) {}

std::chrono::milliseconds MsTimer::duration() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = (now - start_);
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

std::chrono::milliseconds MsTimer::remaining() const
{
    auto duration = this->duration();
    if (duration > period_) {
        return std::chrono::milliseconds(0);
    }
    auto remain = period_ - duration;
    return std::chrono::duration_cast<std::chrono::milliseconds>(remain);
}

std::chrono::milliseconds MsTimer::zero() const noexcept
{
    return period_.zero();
}

bool MsTimer::hasElapsed() const
{
    return remaining() == zero();
}

Timeout::Timeout(
    uint32_t readTimeoutMs,
    uint32_t writeTimeoutMs,
    uint32_t listenTimeoutMs,
    uint32_t readMultiplierMs,
    uint32_t writeMultiplierMs) :
    readTimeoutMs(readTimeoutMs),
    writeTimeoutMs(writeTimeoutMs),
    listenTimeoutMs(listenTimeoutMs),
    readMultiplierMs(readMultiplierMs),
    writeMultiplierMs(writeMultiplierMs) {}

void SerialSubscription::callOnReceive_(const std::vector<uint8_t>& data)
{
    if (onReceive_) {
        onReceive_(data);
    }
}

SerialPortInfo::SerialPortInfo() {}

void SerialPort::startListen_()
{
    readQ_.startRepeat([&, listenTimeoutMs=timeout_.listenTimeoutMs]() {
        if (!waitReadable(listenTimeoutMs)) {
            return;
        }
        size_t avail = available();
        if (avail == 0) {
            return;
        }
        std::vector<uint8_t> buffer;
        buffer.resize(avail);
        size_t r = read(buffer.data(), avail);
        if (r == 0) {
            return;
        }
        buffer.resize(r);
        std::lock_guard lock(subMut_);
        for (auto& sub : subscribers_) {
            if (auto ptr = sub.lock()) {
                ptr->callOnReceive_(buffer);
            }
        }
        std::remove_if(
            subscribers_.begin(),
            subscribers_.end(),
            [](const std::weak_ptr<SerialSubscription>& sub) {
                return sub.expired();
            }
        );
    });
}

void SerialPort::stopListen_()
{
    readQ_.stopRepeat();
}

void SerialPort::subscribe_(const std::shared_ptr<SerialSubscription>& sub)
{
    std::weak_ptr<SerialSubscription> weakPtr(sub);
    std::lock_guard lock(subMut_);
    subscribers_.push_back(weakPtr);
    if (isOpen()) {
        startListen_();
    }
}

std::vector<SerialPortInfo> SerialPort::listPorts() noexcept
{
    return SerialPort::SerialPortImpl::listPorts();
}

SerialPort::SerialPort() : impl_(std::make_unique<SerialPortImpl>()) {}

SerialPort::~SerialPort()
{
    stopListen_();
    try {
        impl_->close();
    } catch (const std::exception& e) {
        std::cerr << "SerialPort dtor exception when closing serial port! " << e.what() << "\n";
    }
}

void SerialPort::setTimeout(const Timeout& timeout) {
    timeout_ = timeout;
}

const Timeout& SerialPort::getTimeout() const {
    return timeout_;
}

void SerialPort::setPort(const std::string& port)
{
    impl_->setPort(port);
}

void SerialPort::setParity(Parity parity)
{
    impl_->setParity(parity);
}

void SerialPort::setStopBits(StopBits stopBits)
{
    impl_->setStopBits(stopBits);
}

void SerialPort::setDataBits(DataBits dataBits)
{
    impl_->setDataBits(dataBits);
}

void SerialPort::setFlowCtrl(FlowControl flowCtrl)
{
    impl_->setFlowCtrl(flowCtrl);
}

void SerialPort::setBaudrate(Baudrate baudrate)
{
    impl_->setBaudrate(baudrate);
}

const std::string& SerialPort::getPort() const noexcept
{
    return impl_->getPort();
}

Parity SerialPort::getParity() const noexcept
{
    return impl_->getParity();
}

StopBits SerialPort::getStopBits() const noexcept
{
    return impl_->getStopBits();
}

DataBits SerialPort::getDataBits() const noexcept
{
    return impl_->getDataBits();
}

FlowControl SerialPort::getFlowCtrl() const noexcept
{
    return impl_->getFlowCtrl();
}

Baudrate SerialPort::getBaudrate() const noexcept
{
    return impl_->getBaudrate();
}

void SerialPort::open()
{
    impl_->open();
    std::lock_guard lock(subMut_);
    if (!subscribers_.empty()) {
        startListen_();
    }
}

bool SerialPort::isOpen() const noexcept
{
    return impl_->isOpen();
}

void SerialPort::close()
{
    stopListen_();
    impl_->close();
}

bool SerialPort::waitReadable(uint32_t timeoutMs)
{
    return impl_->waitReadable(timeoutMs);
}

bool SerialPort::waitWritable(uint32_t timeoutMs)
{
    return impl_->waitWritable(timeoutMs);
}

size_t SerialPort::available() const
{
    return impl_->available();
}

size_t SerialPort::read(uint8_t* buffer, size_t size)
{
    std::lock_guard lock(readMut_);
    return impl_->read(buffer, size, timeout_.readTimeoutMs, timeout_.readMultiplierMs);
}

std::vector<uint8_t> SerialPort::read(size_t size)
{
    std::vector<uint8_t> buffer;
    buffer.resize(size);
    size_t r = read(buffer.data(), size);
    buffer.resize(r);
    return buffer;
}

size_t SerialPort::write(const uint8_t* data, size_t size)
{
    std::lock_guard lock(writeMut_);
    return impl_->write(data, size, timeout_.writeTimeoutMs, timeout_.writeMultiplierMs);
}

size_t SerialPort::write(const std::vector<uint8_t>& data)
{
    return write(data.data(), data.size());
}

std::future<size_t> SerialPort::asyncRead(uint8_t* buffer, size_t size)
{
    return readQ_.enqueue([&, buffer, size]() -> size_t {
        return read(buffer, size);
    });
}

std::future<std::vector<uint8_t>> SerialPort::asyncReadCopy(size_t size)
{
    return readQ_.enqueue([&, size]() -> std::vector<uint8_t> {
        std::vector<uint8_t> buffer;
        buffer.resize(size);
        size_t w = read(buffer.data(), size);
        buffer.resize(w);
        return buffer;
    });
}

std::future<size_t> SerialPort::asyncWrite(const uint8_t* data, size_t size)
{
    return writeQ_.enqueue([&, data, size]() -> size_t {
        return write(data, size);
    });
}

std::future<size_t> SerialPort::asyncWrite(const std::vector<uint8_t>& data)
{
    return asyncWrite(data.data(), data.size());
}

std::future<size_t> SerialPort::asyncWriteCopy(const uint8_t* data, size_t size)
{
    auto ptr = std::make_shared<std::vector<uint8_t>>(data, data + size);
    return writeQ_.enqueue([&, ptr]() -> size_t {
        return write(*ptr);
    });
}

std::future<size_t> SerialPort::asyncWriteCopy(const std::vector<uint8_t>& data)
{
    auto ptr = std::make_shared<std::vector<uint8_t>>(data);
    return writeQ_.enqueue([&, ptr]() -> size_t {
        return write(*ptr);
    });
}

void SerialPort::drain()
{
    std::lock_guard lock(writeMut_);
    impl_->drain();
}

void SerialPort::flush()
{
    std::lock_guard readLock(readMut_);
    std::lock_guard writeLock(writeMut_);
    impl_->flush();
}

void SerialPort::flushInput()
{
    std::lock_guard lock(readMut_);
    impl_->flushInput();
}

void SerialPort::flushOutput()
{
    std::lock_guard lock(writeMut_);
    impl_->flushOutput();
}

void SerialPort::sendBreak(int duration)
{
    impl_->sendBreak(duration);
}

void SerialPort::setBreak(bool level)
{
    impl_->setBreak(level);
}

bool SerialPort::waitForChange()
{
    return impl_->waitForChange();
}

void SerialPort::setRTS(bool level)
{
    impl_->setRTS(level);
}

void SerialPort::setDTR(bool level)
{
    impl_->setDTR(level);
}

bool SerialPort::getCTS() const
{
    return impl_->getCTS();
}

bool SerialPort::getDSR() const
{
    return impl_->getDSR();
}

bool SerialPort::getRI() const
{
    return impl_->getRI();
}

bool SerialPort::getCD() const
{
    return impl_->getCD();
}

} // namespace spp
