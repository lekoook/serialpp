/**
 * @file unix.hpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Header for unix implementation.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#ifndef SPP_UNIX_HPP
#define SPP_UNIX_HPP

#include <cstdint>
#include <serialpp.hpp>
#include <string>

namespace spp
{

class SerialPort::SerialPortImpl {

private:
    std::string port_{};
    Parity parity_{Parity::none};
    StopBits stopBits_{StopBits::stop1};
    DataBits dataBits_{DataBits::data8};
    FlowControl flowCtrl_{FlowControl::none};
    Baudrate baudrate_{Baudrate::br9600};
    bool isOpen_{false};
    int fdSerial_{-1};
    int epollRead_{-1};
    int epollWrite_{-1};

    void configurePort_();
    int closeSerial_();
    int closeEpollRead_();
    int closeEpollWrite_();
    void closeEpoll_();

    static std::vector<SerialPortInfo> getDeviceInfo(const std::vector<std::string>& paths);

public:
    static std::vector<SerialPortInfo> listPorts() noexcept;

    SerialPortImpl();
    ~SerialPortImpl();
    void setPort(const std::string& port);
    void setParity(Parity parity);
    void setStopBits(StopBits stopBits);
    void setDataBits(DataBits dataBits);
    void setFlowCtrl(FlowControl flowCtrl);
    void setBaudrate(Baudrate baudrate);
    const std::string& getPort() const noexcept;
    Parity getParity() const noexcept;
    StopBits getStopBits() const noexcept;
    DataBits getDataBits() const noexcept;
    FlowControl getFlowCtrl() const noexcept;
    Baudrate getBaudrate() const noexcept;
    void open();
    bool isOpen() const noexcept;
    void close();
    int getHandle() const noexcept;
    bool waitReadable(uint32_t timeoutMs);
    bool waitWritable(uint32_t timeoutMs);
    size_t available() const;
    size_t read(uint8_t* buffer, size_t size, uint32_t timeoutMs = 0, uint32_t timeoutMultiplierMs = 0);
    size_t write(const uint8_t* data, size_t size, uint32_t timeoutMs = 0, uint32_t timeoutMultiplierMs = 0);
    void drain();
    void flush();
    void flushInput();
    void flushOutput();
    void sendBreak(int duration);
    void setBreak(bool level = true);
    bool waitForChange();
    void setRTS(bool level = true);
    void setDTR(bool level = true);
    bool getCTS() const;
    bool getDSR() const;
    bool getRI() const;
    bool getCD() const;

};

} // namespace spp

#endif // SPP_UNIX_HPP
