#include <iostream>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#endif

class SerialPort {
private:
#ifdef _WIN32
    HANDLE hSerial;
    COMMTIMEOUTS timeouts;
#else
    int fd;
    termios tty;
#endif

public:
    SerialPort(const char* portName) {
#ifdef _WIN32
        hSerial = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (hSerial == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Error opening serial port");
        }
        GetCommTimeouts(hSerial, &timeouts);
#else
        fd = open(portName, O_RDWR | O_NOCTTY);
        if (fd == -1) {
            throw std::runtime_error("Error opening serial port");
        }
        tcgetattr(fd, &tty);
#endif
    }

    ~SerialPort() {
        close();
    }

    void configure(int baudRate = B115200, unsigned int readTimeout = 1000, unsigned int writeTimeout = 1000) {
#ifdef _WIN32
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        GetCommState(hSerial, &dcbSerialParams);
        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        SetCommState(hSerial, &dcbSerialParams);

        // Configure timeouts
        timeouts.ReadIntervalTimeout = 50; // Maximum time between read chars in ms
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = readTimeout; // Total read timeout in ms
        timeouts.WriteTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = writeTimeout; // Total write timeout in ms
        SetCommTimeouts(hSerial, &timeouts);
#else
        cfsetospeed(&tty, baudRate);
        cfsetispeed(&tty, baudRate);
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 0;  // Non-blocking read
        tty.c_cc[VTIME] = readTimeout / 100; // Set timeout in deciseconds
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;
        tcsetattr(fd, TCSANOW, &tty);
#endif
    }

    int read(char* buffer, unsigned int size) {
#ifdef _WIN32
        DWORD bytesRead;
        if (!ReadFile(hSerial, buffer, size, &bytesRead, NULL)) {
            return -1; // Handle read error
        }
        return bytesRead;
#else
        return ::read(fd, buffer, size);
#endif
    }

    bool write(const char* data, unsigned int length) {
#ifdef _WIN32
        DWORD bytesWritten;
        if (!WriteFile(hSerial, data, length, &bytesWritten, NULL) || bytesWritten != length) {
            return false; // Handle write error
        }
        return true;
#else
        int written = ::write(fd, data, length);
        if (written < 0) {
            return false; // Handle write error
        }
        return written == (int)length;
#endif
    }

    void close() {
#ifdef _WIN32
        CloseHandle(hSerial);
#else
        ::close(fd);
#endif
    }
};
