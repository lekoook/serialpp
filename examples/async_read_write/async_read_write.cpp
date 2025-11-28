/**
 * @file async_read_write.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Example for asynchronous read and write.
 * @details This example illustrates the use of asynchronous reading and writing operations with SerialPort class.
 * For this example to work, the connected serial device should echo back any data bytes it received.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#include <iostream>
#include <serialpp.hpp>

int main(void)
{
    // Create and configure serial port similiar to the synchronous example.
    spp::SerialPort serial;
    serial.setPort("/dev/ttyUSB0");
    serial.setBaudrate(spp::Baudrate::br115200);

    spp::Timeout timeout;
    timeout.readTimeoutMs = 50;
    timeout.writeTimeoutMs = 50;
    timeout.readMultiplierMs = 1;
    timeout.writeMultiplierMs = 1;
    serial.setTimeout(timeout);

    try {
        serial.open();
    } catch (const spp::PortIOException& e) {
        std::cout << "Error connecting to device: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    } catch (const spp::SerialPortException& e) {
        std::cout << "Serial port path is empty: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "Opened serial port: " << serial.getPort() << "\n";
    std::cout << "Waiting 3 seconds for device to get ready..." << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // A call to asynchronous (async) read/write functions returns immediately with a std::future
    // that can be used to get the result of the async operation, as opposed to the synchronous
    // version where the function can block up to the specified timeout duration.
    //
    // Once the async call returns, the read/write operation will begin in background threads.
    // There are TWO threads, one for reading and one for writing. This allows reads and writes to
    // run concurrently.
    //
    // For each thread, each read/write call are queued in a first-in-first-out (FIFO) fashion.
    // For example if there are multiple writes, the writes will be serviced one by one until there are no
    // more writes remaining.
    //

    // Perform an async write. Callers must ensure the data container's lifetime to persist
    // until the async operation is complete (std::future result is available).
    // This is a non-copy async write.
    //
    std::vector<uint8_t> writeData1 = { 0xDE, 0xAD, 0xBE, 0xEF };
    std::future<size_t> wfut1 = serial.asyncWrite(writeData1);
    // Perform some other tasks here in the mean time...
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // Wait to get the result.
    size_t w1 = wfut1.get();
    std::cout << "writeData1 written bytes: " << w1 << "\n";

    // Also performs async write but makes a copy of the data before writing.
    // This copy async write does not require caller to ensure the data container's lifetime to
    // persist until the operation is complete.
    // However, due to the need to copy data, the performance overhead is slightly higher
    // depending on the bytes count.
    std::future<size_t> wfut2;
    {
        uint8_t writeData2[] = { 0x01, 0x02, 0x03, 0x04 };
        wfut2 = serial.asyncWriteCopy(writeData2, sizeof(writeData2));
        // writeData2 gets destroyed when this scope exits.
    }
    // Perform some other tasks here in the mean time...
    std::this_thread::sleep_for(std::chrono::seconds(2));
    size_t w2 = wfut2.get();
    std::cout << "writeData2 written bytes: " << w2 << "\n";

    // Perform an async read. The buffer to contain the received data must persist until the async
    // operation is complete.
    uint8_t readData1[4];
    std::future<size_t> rfut1 = serial.asyncRead(readData1, sizeof(readData1));
    size_t r1 = rfut1.get();
    std::cout << "readData1 read bytes: " << r1 << "\n";
    std::cout << "readData1 bytes: ";
    for (size_t i = 0; i < r1; i++) {
        std::cout << std::hex << (int)readData1[i];
    }
    std::cout << "\n";

    // Also perform an async read but returns a copy of the buffer. The performance overhead is
    // slightly higher depending on the bytes count.
    std::future<std::vector<uint8_t>> rfut2 = serial.asyncReadCopy(4);
    std::vector<uint8_t> readData2 = rfut2.get();
    std::cout << "readData2 read bytes: " << readData2.size() << "\n";
    std::cout << "readData2 bytes: ";
    for (auto b : readData2) {
        std::cout << std::hex << (int)b;
    }
    std::cout << "\n";

    return 0;
}
