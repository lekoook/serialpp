/**
 * @file subscription.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Example for subscription.
 * @details This example illustrates the use of subscription to read data with SerialPort class.
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
    // Subscription mechanism has an additional timeout called listenTimeoutMs.
    // The subscription mechanism runs in a background thread by polling for data with a timeout
    // duration. This timeout is listenTimeoutMs.
    timeout.listenTimeoutMs = 200;
    serial.setTimeout(timeout);

    // As an alternative to async reading, callers can use the subscription mechanism to receive
    // incoming data asynchronously. This is done by supplying a Callable object to the subscribe()
    // function. When data are received, all supplied Callables will be called with the received
    // data.
    //
    // The callable function signature must be:
    //      void (const std::vector<uint8_t>& data)
    //
    // The subscribe() returns a shared_ptr to a SerialSubscription object. The subscription will
    // stay valid (the Callable will be invoked) as long as this SerialSubscription object exists.
    // To unsubscribe, simply destroy this shared_ptr.
    //
    // NOTE: The subscription mechanism runs in the same background thread as async read()
    // functions. Therefore, the Callables will be invoked from that thread's context. Callers
    // should consider protecting their data from multiple thread access such as with the use of
    // std::mutex in the callback.
    std::shared_ptr<spp::SerialSubscription> subscription = serial.subscribe([](const std::vector<uint8_t>& data) {
        // Perform the data processing here.
        std::cout << "Received on thread: " << std::hex << std::this_thread::get_id() << "\n";
        std::cout << "Received data: ";
        for (auto b : data) {
            std::cout << std::hex << (int)b;
        }
        std::cout << "\n";
    });
    std::cout << "Subscribed to incoming data\n";

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

    // Peform multiple async writes to simulate multiple incoming data.
    std::vector<uint8_t> writeData1 = { 0xDE, 0xAD, 0xBE, 0xEF };
    std::future<size_t> wfut1;
    for (size_t i = 0; i < 4; i++) {
        wfut1 = serial.asyncWrite(writeData1);
        std::cout << "Writing data on thread: " << std::hex << std::this_thread::get_id() << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    // Wait to get the result from the last write.
    size_t w1 = wfut1.get();
    std::cout << "writeData1 written bytes: " << w1 << "\n";

    // Sleep here to ensure all data are received before exiting.
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
