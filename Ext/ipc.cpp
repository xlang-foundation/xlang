#include "ipc.h"

#include <windows.h> 
#include <iostream>

namespace X
{
	void IpcServer::run()
	{
		while (true)
		{
            HANDLE hPipe = CreateNamedPipe(
                m_path.c_str(),
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_BYTE |
                PIPE_READMODE_BYTE |
                PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                m_bufferSize,
                m_bufferSize,
                0,
                NULL);
            if (hPipe == INVALID_HANDLE_VALUE)
            {
                break;
            }
            bool fConnected = ConnectNamedPipe(hPipe, NULL) ?
                TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
            if (fConnected)
            {
                IpcSession* pSession = new IpcSession(hPipe,this);
                if (m_newSessionHandler)
                {
                    m_newSessionHandler(m_pHandlerContext,pSession);
                }
                pSession->Start();
            }
            else
            {
                // The client could not connect, so close the pipe. 
                CloseHandle(hPipe);
            }
		}
	}
    bool IpcSession::Send(char* data, int size)
    {
        DWORD dwWrite = 0;
        WriteFile(m_pipe, (char*)&size, sizeof(int), &dwWrite, nullptr);
        if (dwWrite != sizeof(int))
        {
            return false;
        }
        WriteFile(m_pipe, data, size, &dwWrite, nullptr);
        return (dwWrite == size);
    }
    void IpcSession::run()
    {
        int bufSize = m_srv->GetBufferSize();
        char* buf = new char[bufSize];
        while (true)
        {
            DWORD cbBytesRead = 0;
            BOOL fSuccess = FALSE;
            fSuccess = ReadFile(
                m_pipe,
                buf,
                4,
                &cbBytesRead,
                nullptr);
            if (!fSuccess || cbBytesRead == 0)
            {
                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                }
                else
                {
                }
                break;
            }
            if (cbBytesRead == 4)
            {
                unsigned int len = *(unsigned int*)buf;
                fSuccess = ReadFile(
                    m_pipe,
                    buf,
                    len,
                    &cbBytesRead,
                    nullptr);

                buf[cbBytesRead] = 0;
                if (m_dataHandler)
                {
                    m_dataHandler(m_HandlerContext,this, buf, len);
                }
                //std::cout << buf << std::endl;
            }
        }
        delete[] buf;
    }
}