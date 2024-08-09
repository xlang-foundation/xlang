#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
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

    std::atomic<bool> running;
    std::thread readThread;
    std::thread writeThread;
    std::queue<std::vector<char>> writeQueue;
    std::mutex writeMutex;
    std::condition_variable writeCondition;

    std::string portName;
    int baudRate;
    unsigned int readTimeout;
    unsigned int writeTimeout;
    std::function<void(const std::vector<char>&)> m_read_callback;
    void readLoop();
    void writeLoop();
    void reconnect();
public:
    SerialPort(const char* portName);
    ~SerialPort();
    bool open();
    void configure(int baudRate = 115200, unsigned int readTimeout = 1000, unsigned int writeTimeout = 1000);
    void run();
    int read(char* buffer, unsigned int size);
    bool write(const char* data, unsigned int length);
    void setReadCallback(std::function<void(const std::vector<char>&)> callback);
    void asyncWrite(const std::vector<char>& data);
    void close();
};

#endif // SERIAL_PORT_H
