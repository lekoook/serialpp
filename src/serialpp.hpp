/**
 * @file serialpp.hpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Contains the public interfaces for serialpp.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#ifndef SPP_SERIALPP_HPP
#define SPP_SERIALPP_HPP

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace spp {

/**
 * @brief Helper macro to throw exceptions generically.
 *
 */
#define SPP_THROW(exception, message) throw exception(__FILE__, __LINE__, message)

/**
 * @brief Helper macro to throw exceptions with error code generically.
 *
 */
#define SPP_THROW_ERRNO(exception, err, message) throw exception(__FILE__, __LINE__, err, message)

/**
 * @brief Enumeration for configuring parity.
 *
 */
enum class Parity {
    none = 0, /**< None parity. */
    even = 1, /**< Even parity. */
    odd = 2, /**< Odd parity. */
    space = 3, /**< Space parity. */
    mark = 4 /**< Mark parity. */
};

/**
 * @brief Enumeration for configuring stop bits.
 *
 */
enum class StopBits {
    stop1 = 0, /**< One stop bit. */
    stop2 = 1 /**< Two stop bit. */
};

/**
 * @brief Enumeration for configuring data bits.
 *
 */
enum class DataBits {
    data5 = 5, /**< 5 data bits. */
    data6 = 6, /**< 6 data bits. */
    data7 = 7, /**< 7 data bits. */
    data8 = 8 /**< 8 data bits. */
};

/**
 * @brief Enumeration for flow control.
 *
 */
enum class FlowControl {
    none = 0, /**< No flow control. */
    software = 1, /**< Software flow control. */
    hardware = 2 /**< Hardware flow control. */
};

/**
 * @brief Enumeration for baudrate.
 *
 */
enum class Baudrate : uint32_t {
    br1200 = 1200, /**< 1200 */
    br4800 = 4800, /**< 4800 */
    br9600 = 9600, /**< 9600 */
    br19200 = 19200, /**< 19200 */
    br38400 = 38400, /**< 38400 */
    br57600 = 57600, /**< 57600 */
    br115200 = 115200 /**< 115200 */
};

/**
 * @brief Base class for all serial port related exceptions. This exception are used to indicate general errors when 
 * using the serial port classes.
 *
 */
class SerialPortException : public std::exception {

private:
    static constexpr char TYPE_NAME_[] = "SerialPortException";

protected:
    std::string desc_{};

    explicit SerialPortException(const std::string& prefix) noexcept;
    explicit SerialPortException(const std::string& prefix, const std::string& file, int line, int err, const std::string& message) noexcept;
    explicit SerialPortException(const std::string& prefix, const std::string& file, int line, const std::string& message) noexcept;

public:
    SerialPortException() noexcept;
    explicit SerialPortException(const std::string& file, int line, int err, const std::string& message) noexcept;
    explicit SerialPortException(const std::string& file, int line, const std::string& message) noexcept;
    SerialPortException(const SerialPortException& other) noexcept;
    SerialPortException& operator=(const SerialPortException& other) noexcept;
    virtual const char* what() const noexcept;

};

/**
 * @brief Exception that indicates an operation is not allowed when the serial port is closed.
 *
 */
class PortClosedException : public SerialPortException {

private:
    static constexpr char TYPE_NAME_[] = "PortClosedException";

public:
    PortClosedException() noexcept;
    explicit PortClosedException(const std::string& file, int line, int err, const std::string& message) noexcept;
    explicit PortClosedException(const std::string& file, int line, const std::string& message) noexcept;
    PortClosedException(const PortClosedException& other) noexcept;
    PortClosedException& operator=(const PortClosedException& other) noexcept;

};

/**
 * @brief Exception that indicates an error with the serial port input/output operations or configurations.
 *
 */
class PortIOException : public SerialPortException {

private:
    static constexpr char TYPE_NAME_[] = "PortIOException";

public:
    PortIOException() noexcept;
    explicit PortIOException(const std::string& file, int line, int err, const std::string& message) noexcept;
    explicit PortIOException(const std::string& file, int line, const std::string& message) noexcept;
    PortIOException(const PortIOException& other) noexcept;
    PortIOException& operator=(const PortIOException& other) noexcept;

};

class MsTimer {

private:
    std::chrono::time_point<std::chrono::steady_clock> start_{};
    std::chrono::milliseconds period_{};

public:
    MsTimer(std::chrono::milliseconds periodMs);
    MsTimer(uint32_t periodMs);
    std::chrono::milliseconds duration() const;
    std::chrono::milliseconds remaining() const;
    std::chrono::milliseconds zero() const noexcept;
    bool hasElapsed() const;

};

class TaskQueue {

private:
    using WrapperFunc = std::function<void()>;
    std::deque<WrapperFunc> queue_{};
    std::thread thread_{};
    std::mutex mut_{};
    std::condition_variable cv_{};
    bool stop_{};
    std::atomic<bool> repeatStop_{};

    void run_();
    void postRepeat_(WrapperFunc func);

public:
    TaskQueue();

    ~TaskQueue();

    template<typename Callable, typename... Args>
    auto enqueue(Callable&& callable, Args&&... args) -> std::future<decltype(callable(args...))>
    {
        std::function<decltype(callable(args...))()> func = 
            std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
        auto task = std::make_shared<std::packaged_task<decltype(callable(args...))()>>(func);
        std::lock_guard lock(mut_);
        queue_.emplace_back(WrapperFunc([task]() {
            (*task)();
        }));
        cv_.notify_one();
        return task->get_future();
    }
    
    template<typename Callable, typename... Args>
    void startRepeat(Callable&& callable, Args&&... args)
    {
        if (repeatStop_.load()) {
            repeatStop_ = false;
            WrapperFunc func = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
            postRepeat_(func);
            std::lock_guard lock(mut_);
            cv_.notify_one();
        }
    }

    void stopRepeat();

};

/**
 * @brief Structure containing the timeout periods for reading and writing operations.
 * @details The timeout values governs the behaviour of @ref SerialPort::read(),
 * @ref SerialPort::write(), @ref SerialPort::asyncRead(), @ref SerialPort::asyncWrite()
 * and subscriptions.
 *
 * @ref readTimeoutMs is the duration in which sync or async read operations will attempt for,
 * before giving up with a timeout.
 *
 * @ref writeTimeoutMs is the duration in which sync or async write operations will atempt for,
 * before giving up with a timeout.
 *
 * @ref listenTimeoutMs is the duration in which the subscription mechanism will attempt to poll
 * for incoming data before polling again.
 *
 * @ref readMultiplierMs is a multiplier that is multiplied to the number of requested bytes in a
 * sync or async read operation. The product is then added to @ref readTimeoutMs where the sum is
 * the total timeout for the operation.
 *
 * @ref writeMultiplierMs is a multiplier that is multiplied to the number of requested bytes in a
 * sync or async write operation. The product is then added to @ref writeTimeoutMs where the sum is
 * the total timeout for the operation.
 *
 * All timeout durations are in milliseconds.
 *
 */
struct Timeout {
    uint32_t readTimeoutMs{0}; /**< Timeout for read operations. */
    uint32_t writeTimeoutMs{0}; /**< Timeout for write operations. */
    uint32_t listenTimeoutMs{0}; /**< Timeout for subscription polling. */
    uint32_t readMultiplierMs{0}; /**< Timeout bytes multiplier for read operations. */
    uint32_t writeMultiplierMs{0}; /**< Timeout bytes multiplier for write operations. */

    /**
     * @brief Constructs Timeout.
     *
     * @param readTimeoutMs Timeout for read operations.
     * @param writeTimeoutMs Timeout for write operations.
     * @param listenTimeoutMs Timeout for subscription polling.
     * @param readMultiplierMs Timeout bytes multiplier for read operations.
     * @param writeMultiplierMs Timeout bytes multiplier for write operations.
     */
    Timeout(uint32_t readTimeoutMs = 0, uint32_t writeTimeoutMs = 0, uint32_t listenTimeoutMs = 0,
        uint32_t readMultiplierMs = 0, uint32_t writeMultiplierMs = 0);
};

class SerialPort;

/**
 * @brief Represents a subscription to incoming data from serial port.
 *
 */
class SerialSubscription {

friend class SerialPort;

private:
    std::function<void(const std::vector<uint8_t>& data)> onReceive_{};

    template<typename Callable>
    void setOnReceive_(Callable&& callable)
    {
        onReceive_ = callable;
    }

    void callOnReceive_(const std::vector<uint8_t>& data);

};

/**
 * @brief Contains all information about a serial port device.
 *
 */
struct SerialPortInfo {
    std::string name; /**< Name of the serial port device. */
    std::string realName; /**< Real name (resolved symbolic link) of the serial port device. */
    std::string realPath; /**< Real path (resolved symbolic link) of the serial port device. */
    std::string symlinkPath; /**< Path of the serial port device. */
    std::string description; /**< Human readable description about the serial port device. */
    std::string hardwareID; /**< Hardware information about the serial port device. */
    bool isSymlink{false}; /**< True indicates this is a symbolic link, false otherwise. */

    /**
     * @brief Constructs SerialPortInfo.
     *
     */
    SerialPortInfo();
};

/**
 * @brief Primary interface to access a serial port.
 * @details Provides interface to @ref open() and @ref close(), synchronously @ref read() and
 * @ref write(), asynchronously @ref asyncRead() and @ref asyncWrite() data from and to the serial
 * port.
 *
 * A subscription can also be made using @ref subscribe() to be notified via callback
 * whenever data is received from the serial port, as opposed to polling for data via @ref read()
 * or @ref asyncRead().
 *
 * Each instance of SerialPort runs two additional threads in the background, the __read thread__
 * and the __write thread__.
 * Read thread processes the async reading of data from the serial port, write thread processes the
 * async writing of data to the serial port. This allows SerialPort to perform async read and write
 * __concurrently__.
 * For each read and write thread, there exists a First-In-First-Out (FIFO) task queue to
 * process multiple async reads or multiple async writes. Each call to a read/write operation will
 * enqueue the operation in it's own respective queue before being processed by it's own thread.
 *
 * For example, if there are more than one async write called consecutively, the first write will
 * be processed first, followed by the second and then the third, until all remaining write calls
 * are processed. The same mechanism applies to the read thread.
 *
 * For subscriptions, the mechanism works by periodically checking for incoming data and read them
 * whenever possible. This reading is done within the read thread's context.
 *
 * Therefore for async reads/writes and subscriptions, callers must take precautions to protect
 * shared data since data may be shared across multiple threads.
 *
 * @note
 * On async reads and subscriptions: @n
 * In general, it is fine to mix async reads and subscriptions using the same SerialPort instance.
 * This is because the operations are enqueued within a single queue, processed by a single thread.
 * However, since they are independent operations, it is likely that async reads and subscriptions
 * will "steal" data from each other depending on who gets to read the port first. Therefore, it
 * is recommended to stick to either one to avoid unexpected reading behaviour in your application.
 *
 */
class SerialPort {

private:
    class SerialPortImpl;
    std::unique_ptr<SerialPortImpl> impl_{};
    Timeout timeout_{0, 0, 0, 0};
    TaskQueue readQ_{};
    TaskQueue writeQ_{};
    std::mutex readMut_{};
    std::mutex writeMut_{};
    std::mutex subMut_{};
    std::vector<std::weak_ptr<SerialSubscription>> subscribers_{};

    void startListen_();
    void stopListen_();
    void subscribe_(const std::shared_ptr<SerialSubscription>& sub);

public:
    /**
     * @brief Returns a list of serial ports with their information.
     *
     * @return std::vector<SerialPortInfo> List of serial ports.
     */
    static std::vector<SerialPortInfo> listPorts() noexcept;

    /**
     * @brief Constructs SerialPort with default serial port configurations.
     *
     */
    SerialPort();

    /**
     * @brief Destructs SerialPort.
     *
     */
    ~SerialPort();

    /**
     * @brief Sets the timeout.
     *
     * @param timeout Timeout to set.
     */
    void setTimeout(const Timeout& timeout);

    /**
     * @brief Gets the timeout.
     *
     * @return const Timeout& Timeout to get.
     */
    const Timeout& getTimeout() const;

    /**
     * @brief Sets the port.
     *
     * @param port Port to set.
     */
    void setPort(const std::string& port);

    /**
     * @brief Sets the parity.
     *
     * @param parity Parity to set.
     */
    void setParity(Parity parity);

    /**
     * @brief Sets the stop bits.
     *
     * @param stopBits Stop bits to set.
     */
    void setStopBits(StopBits stopBits);

    /**
     * @brief Sets the data bits.
     *
     * @param dataBits Data bits to set.
     */
    void setDataBits(DataBits dataBits);

    /**
     * @brief Sets the flow control.
     *
     * @param flowCtrl Fllow control to set.
     */
    void setFlowCtrl(FlowControl flowCtrl);

    /**
     * @brief Sets the baudrate.
     *
     * @param baudrate Baudrate to set.
     */
    void setBaudrate(Baudrate baudrate);

    /**
     * @brief Gets the port.
     *
     * @return const std::string& Port Port to get.
     */
    const std::string& getPort() const noexcept;

    /**
     * @brief Gets the parity.
     *
     * @return Parity Parity to get.
     */
    Parity getParity() const noexcept;

    /**
     * @brief Gets the stop bits.
     *
     * @return StopBits Stop bits to get.
     */
    StopBits getStopBits() const noexcept;

    /**
     * @brief Gets the data bits.
     *
     * @return DataBits Data bits to get.
     */
    DataBits getDataBits() const noexcept;

    /**
     * @brief Gets the flow control.
     *
     * @return FlowControl Flow control to get.
     */
    FlowControl getFlowCtrl() const noexcept;

    /**
     * @brief Gets the baudrate.
     *
     * @return Baudrate Baudrate to get.
     */
    Baudrate getBaudrate() const noexcept;

    /**
     * @brief Opens the serial port.
     *
     * @throw PortIOException Error is thrown by the system when attempting to create or configure
     * serial ports.
     * @throw SerialPortException Improper serial port configuration values are used.
     */
    void open();

    /**
     * @brief Checks if the serial port is open.
     *
     * @return true Serial port is open.
     * @return false Serial port is closed.
     */
    bool isOpen() const noexcept;

    /**
     * @brief Closes the serial port.
     *
     * @throw PortIOException Error is thrown by the system when attempting to close serial ports.
     */
    void close();

    /**
     * @brief Wait until the serial port is ready for reading or until the specified @p timeoutMs elapses.
     *
     * @param timeoutMs Timeout (milliseconds) duration to use. 
     * @return true Serial port is ready for reading.
     * @return false Timeout elapsed.
     */
    bool waitReadable(uint32_t timeoutMs);

    /**
     * @brief Wait until the serial port is ready for writing or until the specified @p timeoutMs elapses.
     *
     * @param timeoutMs Timeout (milliseconds) duration to use. 
     * @return true Serial port is ready for writing.
     * @return false Timeout elapsed.
     */
    bool waitWritable(uint32_t timeoutMs);

    /**
     * @brief Returns the number of bytes are available in the internal buffer for reading.
     *
     * @return size_t Number of bytes available.
     */
    size_t available() const;

    /**
     * @brief Performs blocking read of @p size bytes from the serial port into @p buffer.
     * @details Undefined behaviour if length of @p buffer is smaller than @p size.
     * @details This function returns when data with length @p size is read or when timeout occurs.
     *
     * @param buffer Buffer to store read data.
     * @param size Number of bytes to read.
     * @return size_t Number of bytes actually read.
     */
    size_t read(uint8_t* buffer, size_t size);

    /**
     * @brief Performs blocking read of @p size bytes from the serial port into a buffer and
     * returning it.
     * @details This function returns when data with length @p size is read or when timeout occurs.
     *
     * @param size Number of bytes to read.
     * @return std::vector<uint8_t> Buffer containing read data. The size indicates the number
     * of bytes actually read.
     */
    std::vector<uint8_t> read(size_t size);

    /**
     * @brief Performs blocking write of @p size bytes of @p data to the serial port.
     * @details Undefined behaviour if length of @p data is smaller than @p size.
     * @details This function returns when @p data with length @p size is written or when timeout
     * occurs.
     *
     * @param data Buffer containing bytes to write.
     * @param size Number of bytes in the buffer.
     * @return size_t Number of bytes actually written.
     */
    size_t write(const uint8_t* data, size_t size);

    /**
     * @brief Performs blocking write of @p data to the serial port.
     * @details This function returns when @p data is written or when timeout occurs.
     *
     * @param data Buffer containing bytes to write.
     * @return size_t Number of bytes actually written.
     */
    size_t write(const std::vector<uint8_t>& data);

    /**
     * @brief Performs non-blocking read of @p size bytes from the serial port into @p buffer.
     * @details Undefined behaviour if length of @p buffer is smaller than @p size.
     * @details The @p buffer will be accessed on a thread separate from the calling thread, caller
     * must ensure the @p buffer is not shared with other threads during reading.
     *
     * @param buffer Buffer to store read data.
     * @param size Number of bytes to read.
     * @return std::future<size_t> Future containing number of bytes actually read.
     */
    std::future<size_t> asyncRead(uint8_t* buffer, size_t size);

    /**
     * @brief Performs non-blocking read of @p size bytes from the serial port into a buffer and
     * returning it.
     *
     * @param size Number of bytes to read.
     * @return std::future<std::vector<uint8_t>> Future containing read data. The vector size
     * indicates the number of bytes actually read.
     */
    std::future<std::vector<uint8_t>> asyncReadCopy(size_t size);

    /**
     * @brief Performs non-blocking write of @p size bytes of @p data to the serial port.
     * @details Undefined behaviour if length of @p data is smaller than @p size.
     * @details The @p data will be accessed on a thread separate from the calling thread, caller
     * must ensure the @p data is not shared with other threads during writing.
     *
     * @param data Buffer containing bytes to write.
     * @param size Number of bytes in the buffer.
     * @return std::future<size_t> Future containing number of bytes actually written.
     */
    std::future<size_t> asyncWrite(const uint8_t* data, size_t size);

    /**
     * @brief Performs non-blocking write of @p data to the serial port.
     * @details The @p data will be accessed on a thread separate from the calling thread, caller
     * must ensure the @p data is not shared with other threads during writing.
     *
     * @param data Buffer containing bytes to write.
     * @return std::future<size_t> Future containing number of bytes actually written.
     */
    std::future<size_t> asyncWrite(const std::vector<uint8_t>& data);

    /**
     * @brief Performs making a copy of @p data then non-blocking write of @p size bytes of copy
     * to the serial port.
     * @details The provided @p data contents will be copied when writing on a thread separate from
     * the calling thread. Callers can modify @p data once this function returns.
     * @details Due to the additional copy, there is a slight overhead depending on @p size.
     *
     * @param data Buffer containing bytes to write.
     * @param size Number of bytes in the buffer.
     * @return std::future<size_t> Future containing number of bytes actually written.
     */
    std::future<size_t> asyncWriteCopy(const uint8_t* data, size_t size);

    /**
     * @brief Performs making a copy of @p data then non-blocking write of copy to the serial port.
     * @details The provided @p data contents will be copied when writing on a thread separate from
     * the calling thread. Callers can modify @p data once this function returns.
     * @details Due to the additional copy, there is a slight overhead depending on size of @p data.
     *
     * @param data Buffer containing bytes to write.
     * @return std::future<size_t> Future containing number of bytes actually written.
     */
    std::future<size_t> asyncWriteCopy(const std::vector<uint8_t>& data);

    /**
     * @brief Subscribes to incoming data from the serial port using @p callable.
     * @details The callback function signature of the supplied @p callable must be:
     * **void (const std::vector<uint8_t>& data)**
     * @details The invocation of @p callable is done in the reading thread's context. Therefore,
     * callers should ensure their data is protected from multiple thread access.
     *
     * @tparam Callable Any callable type. Can be class, function pointers, lambda, etc.
     * @param callable Callable object to be invoked when data are read.
     * @return std::shared_ptr<SerialSubscription> shared_ptr holding the subscription. This pointer
     * can be destroyed or reset to unsubscribe (callable will no longer be invoked).
     */
    template<typename Callable>
    std::shared_ptr<SerialSubscription> subscribe(Callable&& callable)
    {
        auto ptr = std::make_shared<SerialSubscription>();
        ptr->setOnReceive_(callable);
        subscribe_(ptr);
        return ptr;
    }

    /**
     * @brief Blocks until all data that were previously written to the serial port internal buffer
     * are transmitted.
     *
     */
    void drain();

    /**
     * @brief Discards all data that are received but not read (by read operations) and all data
     * that are written (by write operations) but not transmitted in the serial port internal
     * buffer.
     *
     */
    void flush();

    /**
     * @brief Discards all data that are received but not read (by read operations) in the serial
     * port internal buffer.
     *
     */
    void flushInput();

    /**
     * @brief Discards all data that are written (by write operations) but not transmitted
     * in the serial port internal buffer.
     *
     */
    void flushOutput();

    /**
     * @brief Transmits a continuous stream of zero-valued bits for a specific duration.
     *
     * @param duration Duration Duration of the bits, implementation defined. See man page on tcsendbreak().
     */
    void sendBreak(int duration);

    /**
     * @brief Starts or stops transmitting zero-valued bits.
     *
     * @param level True to start, false to stop.
     */
    void setBreak(bool level = true);

    /**
     * @brief Blocks until any of the CTS, DSR, RI or CD bits status changed.
     *
     * @return true Any of the 4 bits changed.
     * @return false Error while waiting.
     */
    bool waitForChange();

    /**
     * @brief Sets or clears the RTS bit status.
     *
     * @param level True to set, false to clear.
     */
    void setRTS(bool level = true);

    /**
     * @brief Sets or clears the DTR bit status.
     *
     * @param level True to set, false to clear.
     */
    void setDTR(bool level = true);
    
    /**
     * @brief Gets the CTS bit status.
     *
     * @return true Bit is set.
     * @return false Bit is cleared.
     */
    bool getCTS() const;

    /**
     * @brief Gets the DSR bit status.
     *
     * @return true Bit is set.
     * @return false Bit is cleared.
     */
    bool getDSR() const;

    /**
     * @brief Gets the RI bit status.
     *
     * @return true Bit is set.
     * @return false Bit is cleared.
     */
    bool getRI() const;

    /**
     * @brief Gets the CD bit status.
     *
     * @return true Bit is set.
     * @return false Bit is cleared.
     */
    bool getCD() const;

};

} // namespace spp

#endif // SPP_SERIALPP_HPP
