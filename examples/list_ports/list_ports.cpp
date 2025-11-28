/**
 * @file list_ports.cpp
 * @author Xavier Lee (kokteng1313@gmail.com)
 * @brief Example for listing serial ports.
 * @details This example will list all serial ports found in the system.
 * @version 0.1.0
 * @date 2025-12-03
 *
 * @copyright Copyright (c) 2025
 */
#include <iostream>
#include <serialpp.hpp>

int main(void)
{
    // Retrieve the list of serial port devices that can be found on the system.
    std::vector<spp::SerialPortInfo> ports = spp::SerialPort::listPorts();

    // Information about each serial port device is contained within the SerialPortInfo structure.
    // The following are contained:
    //  1. name - Name of the device. In Linux, it may be something like "ttyUSB*"/"ttyACM*". If symlinks are used, those will appear here.
    //  2. realName - This will always show the underlying name such as "ttyUSB0". Useful if symlinks are used and you need the underlying device.
    //  3. realPath - Filesystem path to the device. Useful if symlinks are used and you need the underlying device.
    //  4. symlinkPath - If symlinks are used, this will show the symlink filesystem path.
    //  5. description - Human readable description if available.
    //  6. hardwareID - Hardware identifiers. Provided are {idVendor}, {idProduct}, {serial}.
    //  7. isSymlink - Indicates if the device is a symlink.
    for (const spp::SerialPortInfo& port : ports) {
        std::cout
            << "[name]: " << port.name
            << " [realName]: " << port.realName
            << " [realPath]: " << port.realPath
            << " [symlinkPath]: " << port.symlinkPath
            << " [isSymlink]: " << port.isSymlink
            << " [descrption]: " << port.description 
            << " [hardwareID]: " << port.hardwareID << "\n";
    }

    return 0;
}
