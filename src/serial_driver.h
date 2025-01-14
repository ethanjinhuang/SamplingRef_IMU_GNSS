#ifndef SERIAL_DRIVER_H
#define SERIAL_DRIVER_H

#include <iostream>
#include <cstdint>
#include <string.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>


class SerialDriver {
public:
    SerialDriver(const std::string &serial_port_name, std::string &baudrate);
    SerialDriver() = default;
    ~SerialDriver() = default;
    SerialDriver(const SerialDriver &a) = delete;
    SerialDriver &operator=(const SerialDriver &a) = delete;
    SerialDriver(SerialDriver &&a) = delete;
    SerialDriver &operator=(const SerialDriver &&a) = delete;
    
    void open(const std::string &serial_port_name, const std::string &baudrate);
    bool readByte(uint8_t &byte);
    int readBytes(uint8_t *buffer, const unsigned int packetsize);
    // int readString(char* buffer, const unsigned int buffer_size);
    std::string readString();
    bool writeByte(uint8_t byte);
    bool flush();
    void close();

private:
    int file_handle_;
    struct termios config_;

};




#endif // SERIAL_DRIVER_H