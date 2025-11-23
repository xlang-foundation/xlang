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

#include "raw_websocket.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>  // For usleep
#endif

namespace X
{
    namespace WebCore
    {
        // SHA-1 implementation for WebSocket handshake
        static uint32_t sha1_rol(uint32_t value, uint32_t bits)
        {
            return (value << bits) | (value >> (32 - bits));
        }

        static void sha1(const unsigned char* data, size_t len, unsigned char hash[20])
        {
            uint32_t h0 = 0x67452301;
            uint32_t h1 = 0xEFCDAB89;
            uint32_t h2 = 0x98BADCFE;
            uint32_t h3 = 0x10325476;
            uint32_t h4 = 0xC3D2E1F0;

            // Prepare padded message
            size_t paddedLen = ((len + 8) / 64 + 1) * 64;
            std::vector<unsigned char> msg(paddedLen, 0);
            memcpy(msg.data(), data, len);
            msg[len] = 0x80;

            // Add length in bits as big-endian 64-bit integer
            uint64_t bitLen = len * 8;
            for (int i = 0; i < 8; i++)
            {
                msg[paddedLen - 1 - i] = (bitLen >> (i * 8)) & 0xFF;
            }

            // Process 512-bit chunks
            for (size_t chunk = 0; chunk < paddedLen; chunk += 64)
            {
                uint32_t w[80];

                // Break chunk into 16 32-bit big-endian words
                for (int i = 0; i < 16; i++)
                {
                    w[i] = (msg[chunk + i * 4] << 24) |
                        (msg[chunk + i * 4 + 1] << 16) |
                        (msg[chunk + i * 4 + 2] << 8) |
                        (msg[chunk + i * 4 + 3]);
                }

                // Extend to 80 words
                for (int i = 16; i < 80; i++)
                {
                    w[i] = sha1_rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
                }

                // Initialize working variables
                uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

                // Main loop
                for (int i = 0; i < 80; i++)
                {
                    uint32_t f, k;
                    if (i < 20)
                    {
                        f = (b & c) | ((~b) & d);
                        k = 0x5A827999;
                    }
                    else if (i < 40)
                    {
                        f = b ^ c ^ d;
                        k = 0x6ED9EBA1;
                    }
                    else if (i < 60)
                    {
                        f = (b & c) | (b & d) | (c & d);
                        k = 0x8F1BBCDC;
                    }
                    else
                    {
                        f = b ^ c ^ d;
                        k = 0xCA62C1D6;
                    }

                    uint32_t temp = sha1_rol(a, 5) + f + e + k + w[i];
                    e = d;
                    d = c;
                    c = sha1_rol(b, 30);
                    b = a;
                    a = temp;
                }

                // Add chunk hash to result
                h0 += a;
                h1 += b;
                h2 += c;
                h3 += d;
                h4 += e;
            }

            // Produce final hash as big-endian
            for (int i = 0; i < 4; i++)
            {
                hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
                hash[4 + i] = (h1 >> (24 - i * 8)) & 0xFF;
                hash[8 + i] = (h2 >> (24 - i * 8)) & 0xFF;
                hash[12 + i] = (h3 >> (24 - i * 8)) & 0xFF;
                hash[16 + i] = (h4 >> (24 - i * 8)) & 0xFF;
            }
        }

        static std::string base64_encode(const unsigned char* data, size_t len)
        {
            static const char* base64_chars =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

            std::string ret;
            int i = 0;
            unsigned char char_array_3[3];
            unsigned char char_array_4[4];

            while (len--)
            {
                char_array_3[i++] = *(data++);
                if (i == 3)
                {
                    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                    char_array_4[3] = char_array_3[2] & 0x3f;

                    for (i = 0; i < 4; i++)
                        ret += base64_chars[char_array_4[i]];
                    i = 0;
                }
            }

            if (i)
            {
                for (int j = i; j < 3; j++)
                    char_array_3[j] = '\0';

                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

                for (int j = 0; j < i + 1; j++)
                    ret += base64_chars[char_array_4[j]];

                while (i++ < 3)
                    ret += '=';
            }

            return ret;
        }

        RawWebSocket::RawWebSocket()
            : m_socket(INVALID_SOCKET_HANDLE)
            , m_isConnected(false)
        {
#ifdef _WIN32
            static bool wsaInitialized = false;
            if (!wsaInitialized)
            {
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
                wsaInitialized = true;
            }
#endif
        }

        RawWebSocket::~RawWebSocket()
        {
            Close();
        }

        bool RawWebSocket::SendRaw(const void* data, size_t len)
        {
            if (m_socket == INVALID_SOCKET_HANDLE)
                return false;

            const size_t CHUNK_SIZE = 64 * 1024; // 64KB chunks
            size_t totalSent = 0;
            const char* ptr = (const char*)data;

            while (totalSent < len)
            {
                // Send in chunks to avoid filling buffer
                size_t toSend = len - totalSent;
                if (toSend > CHUNK_SIZE)
                    toSend = CHUNK_SIZE;

                // Use MSG_DONTWAIT / non-blocking to avoid blocking
                int sent;
#ifdef _WIN32
                sent = ::send(m_socket, ptr + totalSent, toSend, 0);
                if (sent == SOCKET_ERROR)
                {
                    int error = WSAGetLastError();
                    if (error == WSAEWOULDBLOCK)
                    {
                        // Buffer full, wait a bit and retry
                        Sleep(1);
                        continue;
                    }
                    return false;
                }
#else
                sent = ::send(m_socket, ptr + totalSent, toSend, MSG_DONTWAIT);
                if (sent < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // Buffer full, wait a bit and retry
                        usleep(1000); // 1ms
                        continue;
                    }
                    return false;
                }
#endif

                if (sent == 0)
                    return false; // Connection closed

                totalSent += sent;
            }

            return true;
        }

        bool RawWebSocket::RecvRaw(void* data, size_t len)
        {
            if (m_socket == INVALID_SOCKET_HANDLE)
                return false;

            size_t totalRecv = 0;
            char* ptr = (char*)data;

            while (totalRecv < len)
            {
                int recvd = ::recv(m_socket, ptr + totalRecv, len - totalRecv, 0);
                if (recvd <= 0)
                    return false;
                totalRecv += recvd;
            }

            return true;
        }

        bool RawWebSocket::AcceptHandshake(SocketHandle tcpSocket)
        {
            m_socket = tcpSocket;

            // Set small buffer (256KB) - we'll chunk large messages
            int sendBufSize = 256 * 1024; // 256KB
            if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize)) != 0)
            {
#ifdef _WIN32
                std::cout << "Warning: Failed to set SO_SNDBUF: " << WSAGetLastError() << std::endl;
#else
                std::cout << "Warning: Failed to set SO_SNDBUF: " << errno << std::endl;
#endif
            }

            // Verify what we got
            int actualSize = 0;
#ifdef _WIN32
            int optLen = sizeof(actualSize);
#else
            socklen_t optLen = sizeof(actualSize);
#endif
            if (getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (char*)&actualSize, &optLen) == 0)
            {
#ifdef __linux__
                actualSize /= 2; // Linux returns 2x
#endif
                std::cout << "Socket send buffer: " << (actualSize / 1024) << " KB" << std::endl;
            }

            // Set TCP_NODELAY
            int flag = 1;
            setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

            // Make socket NON-BLOCKING for send
            // We'll check before sending to avoid blocking
#ifdef _WIN32
            u_long mode = 1; // 1 = non-blocking
            ioctlsocket(m_socket, FIONBIO, &mode);
#else
            int flags = fcntl(m_socket, F_GETFL, 0);
            fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
#endif

            // Read HTTP handshake request
            char buffer[4096];
            int recvd = ::recv(m_socket, buffer, sizeof(buffer) - 1, 0);
            if (recvd <= 0)
                return false;

            buffer[recvd] = '\0';

            // Parse Sec-WebSocket-Key
            std::string request(buffer);
            size_t keyPos = request.find("Sec-WebSocket-Key:");
            if (keyPos == std::string::npos)
                return false;

            keyPos += 18; // strlen("Sec-WebSocket-Key:")
            while (keyPos < request.length() && request[keyPos] == ' ')
                keyPos++;

            size_t keyEnd = request.find("\r\n", keyPos);
            if (keyEnd == std::string::npos)
                return false;

            std::string key = request.substr(keyPos, keyEnd - keyPos);

            // Compute accept key
            key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            unsigned char hash[20];
            sha1((const unsigned char*)key.c_str(), key.length(), hash);
            std::string acceptKey = base64_encode(hash, 20);

            // Send handshake response
            std::string response =
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: " + acceptKey + "\r\n"
                "\r\n";

            if (!SendRaw(response.c_str(), response.length()))
                return false;

            m_isConnected = true;
            return true;
        }

        bool RawWebSocket::ReadFrame(std::vector<unsigned char>& payload, bool& isBinary)
        {
            if (!m_isConnected)
                return false;

            // Read frame header (at least 2 bytes)
            unsigned char header[2];
            if (!RecvRaw(header, 2))
                return false;

            bool fin = (header[0] & 0x80) != 0;
            uint8_t opcode = header[0] & 0x0F;
            bool masked = (header[1] & 0x80) != 0;
            uint64_t payloadLen = header[1] & 0x7F;

            // Handle close/ping/pong
            if (opcode == CLOSE)
            {
                m_isConnected = false;
                return false;
            }

            isBinary = (opcode == BINARY);

            // Extended payload length
            if (payloadLen == 126)
            {
                unsigned char extLen[2];
                if (!RecvRaw(extLen, 2))
                    return false;
                payloadLen = (extLen[0] << 8) | extLen[1];
            }
            else if (payloadLen == 127)
            {
                unsigned char extLen[8];
                if (!RecvRaw(extLen, 8))
                    return false;
                payloadLen = 0;
                for (int i = 0; i < 8; i++)
                    payloadLen = (payloadLen << 8) | extLen[i];
            }

            // Masking key
            unsigned char maskKey[4] = { 0 };
            if (masked)
            {
                if (!RecvRaw(maskKey, 4))
                    return false;
            }

            // Read payload
            payload.resize(payloadLen);
            if (payloadLen > 0)
            {
                if (!RecvRaw(payload.data(), payloadLen))
                    return false;

                // Unmask
                if (masked)
                {
                    for (size_t i = 0; i < payloadLen; i++)
                        payload[i] ^= maskKey[i % 4];
                }
            }

            return true;
        }

        bool RawWebSocket::WriteFrame(const void* data, size_t len, bool binary)
        {
            if (!m_isConnected)
                return false;

            // CRITICAL: Check if we can write without blocking
            if (!CanWrite(len + 14)) // +14 for WebSocket frame header
                return false;

            // Build frame header
            std::vector<unsigned char> frame;
            frame.reserve(len + 14); // Max header size + data

            // Byte 0: FIN + opcode
            frame.push_back(0x80 | (binary ? BINARY : TEXT));

            // Byte 1+: Payload length (server doesn't mask)
            if (len <= 125)
            {
                frame.push_back(static_cast<unsigned char>(len));
            }
            else if (len <= 65535)
            {
                frame.push_back(126);
                frame.push_back((len >> 8) & 0xFF);
                frame.push_back(len & 0xFF);
            }
            else
            {
                frame.push_back(127);
                for (int i = 7; i >= 0; i--)
                    frame.push_back((len >> (i * 8)) & 0xFF);
            }

            // Add payload
            const unsigned char* payload = (const unsigned char*)data;
            frame.insert(frame.end(), payload, payload + len);

            // DIRECT SOCKET WRITE - NO BUFFERING
            return SendRaw(frame.data(), frame.size());
        }

        bool RawWebSocket::CanWrite(size_t dataSize)
        {
            if (m_socket == INVALID_SOCKET_HANDLE)
                return false;

            // Quick check: is socket writable?
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(m_socket, &writeSet);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

#ifdef _WIN32
            int result = select(0, NULL, &writeSet, NULL, &timeout);
#else
            int result = select(m_socket + 1, NULL, &writeSet, NULL, &timeout);
#endif

            if (result <= 0 || !FD_ISSET(m_socket, &writeSet))
            {
                // Socket not ready - buffer completely full
                return false;
            }

            // Socket is writable - SendRaw will handle chunking and EAGAIN
            return true;
        }

        void RawWebSocket::Close()
        {
            if (m_socket != INVALID_SOCKET_HANDLE)
            {
                // Send close frame
                unsigned char closeFrame[] = { 0x88, 0x00 }; // FIN + CLOSE, no payload
                SendRaw(closeFrame, 2);

#ifdef _WIN32
                closesocket(m_socket);
#else
                ::close(m_socket);
#endif
                m_socket = INVALID_SOCKET_HANDLE;
            }
            m_isConnected = false;
        }

        // TCP Server implementation
        RawTCPServer::RawTCPServer()
            : m_listenSocket(INVALID_SOCKET_HANDLE)
            , m_port(0)
        {
#ifdef _WIN32
            static bool wsaInitialized = false;
            if (!wsaInitialized)
            {
                WSADATA wsaData;
                WSAStartup(MAKEWORD(2, 2), &wsaData);
                wsaInitialized = true;
            }
#endif
        }

        RawTCPServer::~RawTCPServer()
        {
            Close();
        }

        bool RawTCPServer::Listen(int port)
        {
            m_port = port;

            m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (m_listenSocket == INVALID_SOCKET_HANDLE)
                return false;

            // Set SO_REUSEADDR
            int flag = 1;
            setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(flag));

            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(port);

            if (bind(m_listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR_VALUE)
            {
                Close();
                return false;
            }

            if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR_VALUE)
            {
                Close();
                return false;
            }

            return true;
        }

        SocketHandle RawTCPServer::Accept()
        {
            if (m_listenSocket == INVALID_SOCKET_HANDLE)
                return INVALID_SOCKET_HANDLE;

            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);

            return accept(m_listenSocket, (sockaddr*)&clientAddr, &clientLen);
        }

        void RawTCPServer::Close()
        {
            if (m_listenSocket != INVALID_SOCKET_HANDLE)
            {
#ifdef _WIN32
                closesocket(m_listenSocket);
#else
                ::close(m_listenSocket);
#endif
                m_listenSocket = INVALID_SOCKET_HANDLE;
            }
        }
    }
}