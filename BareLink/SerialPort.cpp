#include "SerialPort.h"

SerialPort::SerialPort(const char* portName, int baudRate, unsigned int readTimeout, unsigned int writeTimeout) :
    running(false), portName(portName) {
    if (!openPort(baudRate, readTimeout, writeTimeout)) {
        throw std::runtime_error("Error opening serial port");
    }
}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::openPort(int baudRate, unsigned int readTimeout, unsigned int writeTimeout) {
#ifdef _WIN32
    hSerial = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    GetCommTimeouts(hSerial, &timeouts);
#else
    fd = open(portName.c_str(), O_RDWR | O_NOCTTY);
    if (fd == -1) {
        return false;
    }
    tcgetattr(fd, &tty);
#endif
    configure(baudRate, readTimeout, writeTimeout);
    running = true;

    writeThread = std::thread(&SerialPort::writeLoop, this);
    readThread = std::thread(&SerialPort::readLoop, this);
    return true;
}

void SerialPort::configure(int baudRate, unsigned int readTimeout, unsigned int writeTimeout) {
    this->baudRate = baudRate;
    this->readTimeout = readTimeout;
    this->writeTimeout = writeTimeout;

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

int SerialPort::read(char* buffer, unsigned int size) {
#ifdef _WIN32
    DWORD bytesRead;
    if (!ReadFile(hSerial, buffer, size, &bytesRead, NULL)) {
        return -1; // Handle read error
    }
    return bytesRead;
#else
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    timeval timeout = { 0, 10000 }; // 10 milliseconds
    int ret = select(fd + 1, &readfds, nullptr, nullptr, &timeout);
    if (ret > 0 && FD_ISSET(fd, &readfds)) {
        int bytesRead = ::read(fd, buffer, size);
        if (bytesRead == 0) {
            // Connection closed by the other side
            return 0;
        }
        return bytesRead;
    }
    else if (ret == 0) {
        return 0; // Timeout
    }
    else {
        return -1; // Error
    }
#endif
}

bool SerialPort::write(const char* data, unsigned int length) {
#ifdef _WIN32
    DWORD bytesWritten;
    if (!WriteFile(hSerial, data, length, &bytesWritten, NULL) || bytesWritten != length) {
        return false; // Handle write error
    }
    return true;
#else
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);

    timeval timeout = { 0, 10000 }; // 10 milliseconds
    int ret = select(fd + 1, nullptr, &writefds, nullptr, &timeout);
    if (ret > 0 && FD_ISSET(fd, &writefds)) {
        int written = ::write(fd, data, length);
        if (written == 0) {
            // Connection closed by the other side
            return false;
        }
        return written == (int)length;
    }
    else if (ret == 0) {
        return false; // Timeout
    }
    else {
        return false; // Error
    }
#endif
}

void SerialPort::readLoop() {
    std::vector<char> packet;
    while (running) {
        std::vector<char> chunk(CHUNK_SIZE); // 1 byte for length + up to 31 bytes of data
        int bytesRead = read(chunk.data(), 1); // First byte is the length of the chunk or ACK

        if (bytesRead == 1) {
            char firstByte = chunk[0];
            if (firstByte ==(char)CHUNK_ACK) { // ACK packet
                std::unique_lock<std::mutex> lock(writeMutex);
                ackReceived = true;
                ackCondition.notify_one();  // Notify writeLoop that ACK was received
                continue;
            }
            else {
                int chunkDataSize = static_cast<unsigned char>(firstByte);
                if (chunkDataSize > CHUNK_DATASIZE) {
                    // Handle invalid chunk size
                    break;
                }
                bytesRead = read(chunk.data() + 1, chunkDataSize);
                if (bytesRead == chunkDataSize) {
                    packet.insert(packet.end(), chunk.begin() + 1, chunk.begin() + 1 + chunkDataSize);

                    // If the packet is complete, process it
                    if (packet.size() >= 4) {
                        int dataSize = *(reinterpret_cast<int*>(packet.data()));
                        if (packet.size() - 4 == dataSize) {
                            std::vector<char> fullData(packet.begin() + 4, packet.end());
                            m_read_callback(fullData);
                            packet.clear();
                        }
                    }
                }
                // Send ACK for each received chunk
                std::vector<char> ackPacket = { (char)CHUNK_ACK };
                {
                    std::lock_guard<std::mutex> lock(writeMutex);
                    writeQueue.push(ackPacket);
                }
                writeCondition.notify_one();
            }
        }
    }
}

void SerialPort::writeLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(writeMutex);
        writeCondition.wait(lock, [this]() { return !writeQueue.empty() || !running; });

        while (!writeQueue.empty()) {
            std::vector<char> packet = writeQueue.front();
            writeQueue.pop();
            lock.unlock();
            // If the packet is just an ACK, send it immediately as a single byte
            if (packet.size() == 1 && packet[0] == (char)CHUNK_ACK) {
                write(&packet[0], 1); // Send just the ACK byte
                lock.lock();
                continue;
            }

            size_t packetSize = packet.size();
            size_t offset = 0;

            while (offset < packetSize) { // Include the 4-byte header in the chunks
                size_t chunkSize = (packetSize - offset > CHUNK_DATASIZE) ? CHUNK_DATASIZE : (packetSize - offset);
                std::vector<char> chunk = createChunk(packet.data(), offset, chunkSize);
                sendChunk(chunk);
                offset += chunkSize;

                // Wait for acknowledgment before sending the next chunk
                std::unique_lock<std::mutex> ackLock(writeMutex);
                ackCondition.wait(ackLock, [this]() { return ackReceived; });
                ackReceived = false;  // Reset ACK flag for the next chunk
            }
            lock.lock();
        }
    }
}

std::vector<char> SerialPort::createChunk(const char* data, size_t offset, size_t chunkSize) {
    std::vector<char> chunk(chunkSize + 1); // 1 byte for length + chunk data
    chunk[0] = static_cast<char>(chunkSize); // Length of the chunk
    std::memcpy(chunk.data() + 1, data + offset, chunkSize);
    return chunk;
}

void SerialPort::sendChunk(const std::vector<char>& chunk) {
    bool written = write(chunk.data(), chunk.size());
    if (!written) {
        // Handle write error or connection closure, attempt to reconnect
        std::cerr << "Failed to send chunk, handling error..." << std::endl;
        // reconnect();  // Uncomment and implement this if needed
    }
}

void SerialPort::asyncRead(std::function<void(const std::vector<char>&)> callback) {
    m_read_callback = callback;
}

void SerialPort::asyncWrite(const std::vector<char>& data) {
    std::vector<char> packet(4 + data.size());
    int dataSize = data.size();
    std::memcpy(packet.data(), &dataSize, 4);  // 4-byte header for the length of the data
    std::memcpy(packet.data() + 4, data.data(), data.size());

    {
        std::lock_guard<std::mutex> lock(writeMutex);
        writeQueue.push(packet);
    }
    writeCondition.notify_one();
}

void SerialPort::close() {
    running = false;
    writeCondition.notify_one();
    ackCondition.notify_one();
    if (readThread.joinable()) {
        readThread.join();
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }
#ifdef _WIN32
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
#else
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
#endif
}
