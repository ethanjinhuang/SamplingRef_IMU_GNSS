#include "serial_driver.h"

SerialDriver::SerialDriver(const std::string &serial_port_name, std::string &baudrate) {
    open(serial_port_name, baudrate);
}

void SerialDriver::open(const std::string &serial_port_name, const std::string &baudrate) {

    file_handle_ = ::open(serial_port_name.data(), O_RDWR | O_NOCTTY | O_NDELAY);
    
    // Check Serial Open Seccessful
    if (file_handle_ < 0) {
        throw std::runtime_error{std::string{strerror(errno)} + ": " + serial_port_name};
    }
    
    // Check Serial Port is TTY Device
    if (!isatty(file_handle_)) {
        throw std::runtime_error{"Serial port is not TTY device"};
    }
    
    // Get Current Serial Config
    if (tcgetattr(file_handle_, &config_) < 0) {
        throw std::runtime_error{"Could not retrieve current serial config"};
    }

    //
    // Input flags - Turn off input processing
    //
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    //
    config_.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);

    //
    // Output flags - Turn off output processing
    //
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    //
    // config_.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    config_.c_oflag = 0;

    //
    // No line processing
    //
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    //
    config_.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    //
    // Turn off character processing
    //
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input
    //
    config_.c_cflag &= ~(CSIZE | PARENB);
    config_.c_cflag |= CS8;

    //
    // Zero input byte is enough to return from read()
    // Inter-character timer off
    // I.e. no blocking: return immediately with what is available.
    //
    config_.c_cc[VMIN] = 0;
    config_.c_cc[VTIME] = 0;
    //
    // Communication speed (simple version, using the predefined
    // constants)
    //

    bool baudrate_set = false;
    
    if (baudrate == "115200") {
        if (cfsetispeed(&config_, B115200) < 0 || cfsetospeed(&config_, B115200) < 0) {
            throw std::runtime_error{"Could not set desired baud rate"};
        }
        baudrate_set = true;
    } else if (baudrate == "377400") {
        if (cfsetispeed(&config_, 377400) < 0 || cfsetospeed(&config_, 377400) < 0) {
            throw std::runtime_error{"Could not set desired baud rate"};
        }
        baudrate_set = true;
    } else if (baudrate == "460800") {
        if (cfsetispeed(&config_, B460800) < 0 || cfsetospeed(&config_, B460800) < 0) {
            throw std::runtime_error{"Could not set desired baud rate"};
        }
        baudrate_set = true;
    } else if (baudrate == "921600") {
        if (cfsetispeed(&config_, B921600) < 0 || cfsetospeed(&config_, B921600) < 0) {
            throw std::runtime_error{"Could not set desired baud rate"};
        }
        baudrate_set = true;
    } else if (baudrate == "1843200") {
        if (cfsetispeed(&config_, 1843200) < 0 || cfsetospeed(&config_, 1843200) < 0) {
            throw std::runtime_error{"Could not set desired baud rate"};
        }
        baudrate_set = true;
    }
    // Check Baudrate Set Successful
    if (!baudrate_set) {
        throw std::runtime_error{"Desired baud rate is not supported"};
    }


    // Apply configuration
    if (tcsetattr(file_handle_, TCSAFLUSH, &config_) < 0) {
        throw std::runtime_error{"Could not apply serial port configuration"};
    }
}

void SerialDriver::close() {
    ::close(file_handle_);
}

bool SerialDriver::readByte(uint8_t &byte) {
    int bytes_read = ::read(file_handle_, &byte, 1);
    // if (bytes_read <= 0) {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //     bytes_read = ::read(file_handle_, &byte, 1);
    //     if (bytes_read == 1) {
    //         return true;
    //     } else {
    //         throw std::runtime_error{"ReadPacket error: " + std::string{strerror(errno)}};
    //     }
        
    // }
    return bytes_read == 1;
}

int SerialDriver::readBytes(uint8_t *buffer, const unsigned int packetsize) {
    int bytes_read = ::read(file_handle_, buffer, packetsize);
    if (bytes_read < 0) {
        throw std::runtime_error{"ReadPacket error: " + std::string{strerror(errno)}};
    }
    return bytes_read;
}

std::string SerialDriver::readString() {
    std::string data;
    char buffer[256];
    ssize_t bytes_read;
    bool crReceived = false;
    
    while (!crReceived) {
        bytes_read = ::read(file_handle_, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            for (int i = 0; i< bytes_read; i++) {
                if (buffer[i] == '\r') {
                    crReceived = true;
                    break;
                } else {
                    data += buffer[i];
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    return data;
}


bool SerialDriver::writeByte(uint8_t byte) {
    // std::cout << "File Handle: " << file_handle_ << std::endl;
    int bytes_written = ::write(file_handle_, &byte, 1);
    if (bytes_written < 0) {
        throw std::runtime_error{"WriteByte error:" + std::string{strerror(errno)}};
    }
    return bytes_written == 1;
}

bool SerialDriver::flush() {
    return tcflush(file_handle_, TCIOFLUSH) == 0;
}




