/**
 * @file read_write.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Example for synchronous read and write.
 * @details This example illustrates the use of synchronous reading and writing operations with SerialPort class.
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
    // Create a SerialPort instance and configure accordingly.
    spp::SerialPort serial;
    serial.setPort("/dev/ttyUSB0");                     // Sets the device path.
    serial.setBaudrate(spp::Baudrate::br115200);        // Sets the baudrate to be used.

    // Timeout structure configures the reading/writing timeout behaviour.
    // All timeouts are in milliseconds.
    //
    // - readTimeoutMs
    // This timeout starts when a call is made to any of the read() functions. If the duration elapses before the 
    // requested number of bytes are read, the call returns. Remaining bytes are not read.
    //
    // - writeTimeoutMs
    // This timeout starts when a call is made to any of the write() functions. If the duration elapses before the 
    // requested number of bytes are written, the call returns. Remaining bytes are not written.
    //
    // - readMultiplierMs
    // This is a milliseconds multiplier that is multiplied with the requested number of bytes when making a call to a 
    // read() function. The product of the two is then added to readTimeoutMs where the total sum is used as timeout 
    // for the read(). This allows the flexibility to use a longer timeout when reading large data.
    //
    // - writeMultiplierMs
    // This is a milliseconds multiplier that is multiplied with the requested number of bytes when making a call to a 
    // write() function. The product of the two is then added to writeTimeoutMs where the total sum is used as timeout 
    // for the write(). This allows the flexibility to use a longer timeout when writing large data.
    spp::Timeout timeout;
    timeout.readTimeoutMs = 50;
    timeout.writeTimeoutMs = 50;
    timeout.readMultiplierMs = 1;
    timeout.writeMultiplierMs = 1;
    serial.setTimeout(timeout);

    // Open the serial port for read/write.
    // If read/write operations are attempted before a port is open, exceptions will be thrown.
    //
    // open() can throw:
    //
    // - PortIOException
    // If there is an issue with opening a connection to the serial port.
    // One reason could be the device port path used is incorrect or the device is not connected.
    //
    // - SerialPortException
    // If the provided device port path is empty string.
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
    // Short sleep for device to get ready.
    std::cout << "Waiting 3 seconds for device to get ready..." << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Data can be written to the serial port synchronously.
    // This means the call to write() will block until either:
    // 1. All data bytes are written.
    // 2. Timeout elapsed.
    //
    // If timeout elapses before all data is written, write() will return the total number 
    // of bytes that were successfully written. This can be less than the requested amount.

    // One way to write in data synchronously by using std::vector.
    std::vector<uint8_t> writeData1 = { 0xDE, 0xAD, 0xBE, 0xEF };
    size_t w1 = serial.write(writeData1);
    std::cout << "writeData1 written bytes: " << w1 << "\n";

    // Another way to write in data synchronously by using primitive array.
    uint8_t writeData2[] = { 0x01, 0x02, 0x03, 0x04 };
    size_t w2 = serial.write(writeData2, sizeof(writeData2));
    std::cout << "writeData2 written bytes: " << w2 << "\n";

    // Data can be read from the serial port synchronously.
    // This means the call to read() will block until either:
    // 1. All data bytes are received based on the requested bytes count.
    // 2. Timeout elapsed.
    //
    // If timeout elapses before all data is read, read() will return the total number 
    // of bytes that were successfully read. This can be less than the requested number.

    // One way to read in data synchronously by using std::vector.
    std::vector<uint8_t> readData1 = serial.read(4);
    std::cout << "readData1 read bytes: " << readData1.size() << "\n";
    std::cout << "readData1 bytes: ";
    for (auto b : readData1) {
        std::cout << std::hex << (int)b;
    }
    std::cout << "\n";

    // Another way to read in data synchronously by using primitive array.
    uint8_t readData2[4];
    size_t r2 = serial.read(readData2, sizeof(readData2));
    std::cout << "readData2 read bytes: " << r2 << "\n";
    std::cout << "readData2 bytes: ";
    for (size_t i = 0; i < r2; i++) {
        std::cout << std::hex << (int)readData2[i];
    }
    std::cout << "\n";

    return 0;
}
