#include "SerialPort.h"

SerialPort::SerialPort(const char* portName) : running(false), portName(portName) {

}

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open() {
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

void SerialPort::run() {
    running = true;
    writeThread = std::thread(&SerialPort::writeLoop, this);
    readThread = std::thread(&SerialPort::readLoop, this);
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
    while (running) {
        std::vector<char> buffer(4);
        int bytesRead = read(buffer.data(), buffer.size());
        if (bytesRead == 4) {
            int dataSize = *(reinterpret_cast<int*>(buffer.data()));
            buffer.resize(dataSize);
            bytesRead = read(buffer.data(), buffer.size());
            if (bytesRead == dataSize) {
                m_read_callback(buffer);
            }
        }
        else if (bytesRead == 0) {
            // Connection closed by the other side, attempt to reconnect
            //reconnect();
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

            bool written = write(packet.data(), packet.size());
            if (!written) {
                // Handle write error or connection closure, attempt to reconnect
                //reconnect();
            }
        }
    }
}

void SerialPort::reconnect() {
    std::cout << "Attempting to reconnect..." << std::endl;
    close();

    while (running) {
        if (open()) {
            run();
            std::cout << "Reconnected to serial port" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Retry every 1 second
    }
}

void SerialPort::setReadCallback(std::function<void(const std::vector<char>&)> callback) {
    m_read_callback = callback;
}

void SerialPort::asyncWrite(const std::vector<char>& data) {
    std::vector<char> packet(4 + data.size());
    int dataSize = data.size();
    std::memcpy(packet.data(), &dataSize, 4);
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
