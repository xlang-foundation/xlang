#ifndef DEVICE_LOOP_H
#define DEVICE_LOOP_H

#include <vector>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <unordered_map>
#include "SerialPortBM.h"
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
