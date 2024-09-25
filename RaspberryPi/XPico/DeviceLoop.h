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

#ifndef DEVICE_LOOP_H
#define DEVICE_LOOP_H

#include <vector>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <unordered_map>
#include "serial_port_bm.h"
#include "xlang.h"

class DeviceLoop {
    enum class CommandType { LoadCode, RunCode, StopCodeRun };
public:
    DeviceLoop(uart_inst_t* uart, uint txPin, uint rxPin);
    void start();
private:
    std::unordered_map<unsigned long long, X::Value> m_moduleMap;
    static void onDataReceived(const std::vector<char>& data, void* context);
    SerialPortBM m_serialPort;
    void processCommand(const std::vector<char>& command);
    std::vector<char> ConvertToBytes(X::Value& val);
    bool LoadCode(std::string& moduleName,std::string& code,unsigned long long& moduleKey);
};

#endif // DEVICE_LOOP_H
