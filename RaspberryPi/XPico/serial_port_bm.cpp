#include <string.h>
#include "SerialPortBM.h"

SerialPortBM::SerialPortBM(uart_inst_t* uart, uint txPin, uint rxPin)
    : uart(uart), txPin(txPin), rxPin(rxPin) {
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

bool SerialPortBM::readChunk(std::vector<char>& chunk) {
    char lengthByte;
    if (read(&lengthByte, 1) == 1) {
        int chunkSize = static_cast<unsigned char>(lengthByte);
        if (chunkSize > 31) {
            // Invalid chunk size
            return false;
        }
        chunk.resize(chunkSize);
        return read(chunk.data(), chunkSize) == chunkSize;
    }
    return false;
}

bool SerialPortBM::sendChunk(const std::vector<char>& chunk) {
    return write(chunk.data(), chunk.size());
}

std::vector<char> SerialPortBM::createChunk(const char* data, size_t offset, size_t chunkSize) {
    std::vector<char> chunk(chunkSize + 1); // 1 byte for length + chunk data
    chunk[0] = static_cast<char>(chunkSize); // Length of the chunk
    memcpy(chunk.data() + 1, data + offset, chunkSize);
    return chunk;
}

void SerialPortBM::asyncRead(DataCallback callback, void* context) {
    std::vector<char> packet;
    while (true) {
        std::vector<char> chunk;
        if (readChunk(chunk)) {
            packet.insert(packet.end(), chunk.begin(), chunk.end());

            // Send acknowledgment for each received payload
            char ack = 0x06; // Example ACK byte
            write(&ack, 1);

            // If the packet is complete, process it
            if (packet.size() >= 4) {
                int dataSize = *(reinterpret_cast<int*>(packet.data()));
                if (packet.size() - 4 == dataSize) {
                    std::vector<char> fullData(packet.begin() + 4, packet.end());
                    callback(fullData, context);
                    packet.clear();
                }
            }
        }
    }
}

void SerialPortBM::asyncWrite(const std::vector<char>& data) {
    std::vector<char> packet(4 + data.size());
    int dataSize = data.size();
    memcpy(packet.data(), &dataSize, 4); // 4-byte header for the length of the data
    memcpy(packet.data() + 4, data.data(), data.size());

    size_t offset = 0;
    while (offset < dataSize + 4) { // Include the 4-byte header in the chunks
        size_t chunkSize = (dataSize + 4 - offset > 31) ? 31 : (dataSize + 4 - offset);
        std::vector<char> chunk = createChunk(packet.data(), offset, chunkSize);
        if (!sendChunk(chunk)) {
            // Handle write error or connection closure
            reconnect();
        }

        // Wait for acknowledgment before sending the next chunk
        char ackBuffer;
        int ackBytes = read(&ackBuffer, 1);
        if (ackBytes != 1 || ackBuffer != 0x06) {  // Check for the ACK byte (0x06)
            std::cerr << "Failed to receive ACK, retrying..." << std::endl;
            offset -= chunkSize; // Retry the same chunk
        }
        else {
            offset += chunkSize;
        }
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
