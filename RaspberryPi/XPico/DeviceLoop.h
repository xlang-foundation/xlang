#ifndef DEVICE_LOOP_H
#define DEVICE_LOOP_H

#include <vector>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "serial_port_bm.h"
#include "xlang.h"

class DeviceLoop {
public:
    DeviceLoop(uart_inst_t* uart, uint txPin, uint rxPin);
    void start();
private:
    SerialPortBM m_serialPort;
    void processCommand(const std::vector<char>& command);
    std::vector<char> ConvertToBytes(X::Value& val);
};

#endif // DEVICE_LOOP_H
