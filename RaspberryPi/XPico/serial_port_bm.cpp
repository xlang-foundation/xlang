#include "serial_port_bm.h"

SerialPortBM::SerialPortBM(uart_inst_t* uart, uint txPin, uint rxPin) : uart(uart), txPin(txPin), rxPin(rxPin) {
    stdio_init_all();
    uart_init(uart, baudRate);
    gpio_set_function(txPin, GPIO_FUNC_UART);
    gpio_set_function(rxPin, GPIO_FUNC_UART);
    configure(baudRate, readTimeout, writeTimeout);
}

void SerialPortBM::configure(uint baudRate, uint readTimeout, uint writeTimeout) {
    this->baudRate = baudRate;
    this->readTimeout = readTimeout;
    this->writeTimeout = writeTimeout;
    uart_set_baudrate(uart, baudRate);
    uart_set_fifo_enabled(uart, true);
}

int SerialPortBM::read(char* buffer, unsigned int size) {
    int bytesRead = 0;
    absolute_time_t start = get_absolute_time();
    while (bytesRead < size) {
        if (uart_is_readable(uart)) {
            buffer[bytesRead++] = uart_getc(uart);
        }
        if (absolute_time_diff_us(start, get_absolute_time()) > readTimeout * 1000) {
            break; // Timeout
        }
    }
    return bytesRead;
}

bool SerialPortBM::write(const char* data, unsigned int length) {
    int bytesWritten = 0;
    absolute_time_t start = get_absolute_time();
    while (bytesWritten < length) {
        if (uart_is_writable(uart)) {
            uart_putc(uart, data[bytesWritten++]);
        }
        if (absolute_time_diff_us(start, get_absolute_time()) > writeTimeout * 1000) {
            return false; // Timeout
        }
    }
    return true;
}

void SerialPortBM::asyncRead(DataCallback callback) {
    while (true) {
        std::vector<char> buffer(4);
        int bytesRead = read(buffer.data(), buffer.size());
        if (bytesRead == 4) {
            int dataSize = *(reinterpret_cast<int*>(buffer.data()));
            buffer.resize(dataSize);
            bytesRead = read(buffer.data(), buffer.size());
            if (bytesRead == dataSize) {
                callback(buffer);
            }
        }
        else if (bytesRead == 0) {
            // Connection closed by the other side, attempt to reconnect
            reconnect();
        }
    }
}

void SerialPortBM::asyncWrite(const std::vector<char>& data) {
    std::vector<char> packet(4 + data.size());
    int dataSize = data.size();
    std::memcpy(packet.data(), &dataSize, 4);
    std::memcpy(packet.data() + 4, data.data(), data.size());

    bool written = write(packet.data(), packet.size());
    if (!written) {
        // Handle write error or connection closure
        reconnect();
    }
}

void SerialPortBM::reconnect() {
    close();
    while (true) {
        stdio_init_all();
        uart_init(uart, baudRate);
        gpio_set_function(txPin, GPIO_FUNC_UART);
        gpio_set_function(rxPin, GPIO_FUNC_UART);
        if (uart_is_enabled(uart)) {
            return;
        }
        sleep_ms(1000); // Retry every 1 second
    }
}

void SerialPortBM::close() {
    uart_deinit(uart);
}