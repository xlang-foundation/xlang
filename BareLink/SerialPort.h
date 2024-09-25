/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
    bool openPort();

public:
    SerialPort(const char* portName);
    ~SerialPort();
    void configure(int baudRate = 115200, unsigned int readTimeout = 1000, unsigned int writeTimeout = 1000);
    inline bool open()
    {
        return openPort();
    }
    int read(char* buffer, unsigned int size);
    bool write(const char* data, unsigned int length);
    void asyncRead(std::function<void(const std::vector<char>&)> callback);
    void asyncWrite(const std::vector<char>& data);
    void close();
};

#endif // SERIAL_PORT_H
