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

#ifndef SERIAL_PORT_BM_H
#define SERIAL_PORT_BM_H

#include <vector>
#include "pico/stdlib.h"
#include "hardware/uart.h"

class SerialPortBM {
private:
    uart_inst_t* uart;
    uint txPin;
    uint rxPin;
    uint baudRate;
    uint readTimeout;
    uint writeTimeout;

    void reconnect();

public:
    typedef void (*DataCallback)(const std::vector<char>& data, void* context);

    SerialPortBM(uart_inst_t* uart, uint txPin, uint rxPin);
    void configure(uint baudRate = 115200, uint readTimeout = 1000, uint writeTimeout = 1000);
    int read(char* buffer, unsigned int size);
    bool write(const char* data, unsigned int length);
    void asyncRead(DataCallback callback,void* context);
    void asyncWrite(const std::vector<char>& data);
    void close();
};

#endif // SERIAL_PORT_BM_H
