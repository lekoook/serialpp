# serialpp

`serialpp` is a C++ library used to communicate with serial devices. It includes the opening/closing of serial connections, _synchronous_ reading/writing, _asynchronous_ reading/writing and a _subscription based_ API to receive incoming data.

## Features

- Serial port configurations (open/close port, set baudrate, parity, stop bits, etc.)
- List serial devices in system
- Synchronous (blocking) reading/writing
- Asynchronous (non-blocking) reading/writing
- Subscription based reading

## Dependencies

Required:

- [cmake](https://cmake.org/) build system
- C++17 and above
- C++ `STL`

## Platform

Currently, the only supported platform is `Linux`.

In future, other `Unix`-like and `Windows` platforms may be implemented and supported.

## Building

To build with `cmake`:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

This will build the library and examples.

## Examples

- Listing serial ports: list_ports.cpp
- Synchronous read/write: read_write.cpp
- Asynchronous read/write: async_read_write.cpp
- Subscription: subscription.cpp

## Quick Example (Asynchronous)

```c++
#include <serialpp.hpp>

int main(void)
{
    spp::SerialPort serial;
    serial.setPort("/dev/ttyUSB0");
    serial.setBaudrate(spp::Baudrate::br115200);
    serial.open();

    std::vector<uint8_t> write = { 0xDE, 0xAD, 0xBE, 0xEF };
    std::future<size_t> wfut = serial.asyncWrite(write);
    // Perform some other tasks here in the mean time...
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // Wait to get the result for number of bytes written.
    size_t w = wfut.get();

    uint8_t readData1[4];
    std::future<size_t> rfut = serial.asyncRead(readData1, sizeof(readData1));
    // Wait to get the result for number of bytes read.
    size_t r = rfut.get();

    return 0;
}
```

## TODOs

- Other Unix and Windows support
- Tests

## Authors

- Xavier Lee (kokteng1313@gmail.com)

## License

MIT License (See [LICENSE](LICENSE))
