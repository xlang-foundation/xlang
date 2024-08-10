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
    bool readChunk(std::vector<char>& chunk);
    bool sendChunk(const std::vector<char>& chunk);
    std::vector<char> createChunk(const char* data, size_t offset, size_t chunkSize);

public:
    typedef void (*DataCallback)(const std::vector<char>& data, void* context);

    SerialPortBM(uart_inst_t* uart, uint txPin, uint rxPin);
    void configure(uint baudRate = 115200, uint readTimeout = 1000, uint writeTimeout = 1000);
    int read(char* buffer, unsigned int size);
    bool write(const char* data, unsigned int length);
    void asyncRead(DataCallback callback, void* context);
    void asyncWrite(const std::vector<char>& data);
    void close();
};

#endif // SERIAL_PORT_BM_H
