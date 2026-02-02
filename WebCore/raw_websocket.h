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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
typedef SOCKET SocketHandle;
#define INVALID_SOCKET_HANDLE INVALID_SOCKET
#define SOCKET_ERROR_VALUE SOCKET_ERROR
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SocketHandle;
#define INVALID_SOCKET_HANDLE -1
#define SOCKET_ERROR_VALUE -1
#define SIOCOUTQ TIOCOUTQ
#endif

namespace X
{
    namespace WebCore
    {
        // Raw TCP WebSocket implementation - NO BOOST, NO INTERNAL BUFFERING
        class RawWebSocket
        {
        private:
            SocketHandle m_socket;
            bool m_isConnected;

            // WebSocket frame opcodes
            enum Opcode
            {
                CONTINUATION = 0x0,
                TEXT = 0x1,
                BINARY = 0x2,
                CLOSE = 0x8,
                PING = 0x9,
                PONG = 0xA
            };

            bool SendRaw(const void* data, size_t len);
            bool RecvRaw(void* data, size_t len);

        public:
            RawWebSocket();
            ~RawWebSocket();

            // Accept WebSocket from TCP socket
            bool AcceptHandshake(SocketHandle tcpSocket);

            // Read WebSocket frame
            bool ReadFrame(std::vector<unsigned char>& payload, bool& isBinary);

            // Write WebSocket frame - DIRECT TCP WRITE, NO BUFFERING
            bool WriteFrame(const void* data, size_t len, bool binary = true);

            // Check if socket send buffer has space (prevents blocking)
            bool CanWrite(size_t dataSize);

            // Close WebSocket
            void Close();

            bool IsConnected() const { return m_isConnected; }
            SocketHandle GetSocket() const { return m_socket; }
        };

        // TCP Server for accepting connections
        class RawTCPServer
        {
        private:
            SocketHandle m_listenSocket;
            int m_port;

        public:
            RawTCPServer();
            ~RawTCPServer();

            bool Listen(int port);
            SocketHandle Accept();
            void Close();
        };
    }
}