#include "ipc.h"

#include <windows.h> 
#include <iostream>
#include "event.h"

namespace X
{
	void IpcServer::run()
	{
        X::Event* pEvt = X::EventSystem::I().Register("IPC.Session");
        if (pEvt)
        {
            pEvt->Release();
        }
		while (true)
		{
            HANDLE hPipe = CreateNamedPipe(
                m_path.c_str(),
                PIPE_ACCESS_DUPLEX|
                FILE_FLAG_OVERLAPPED,
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
            HANDLE hEvt = CreateEvent(NULL, TRUE, TRUE, NULL);
            IpcSession* pSession = new IpcSession(hPipe, this, hEvt);
            OVERLAPPED* overlap = (OVERLAPPED*)pSession->GetOverlaped();
            BOOL fConnected = ConnectNamedPipe(hPipe, overlap);
            if (fConnected)
            {
                if (m_newSessionHandler)
                {
                    m_newSessionHandler(m_pHandlerContext,pSession);
                }
                pSession->Start();
            }
            else
            {
                WaitForSingleObject(hEvt, INFINITE);
                DWORD cbBytes = 0;
                BOOL bOK = GetOverlappedResult(hPipe, overlap, &cbBytes, FALSE);
                if (bOK)
                {
                    if (m_newSessionHandler)
                    {
                        m_newSessionHandler(m_pHandlerContext, pSession);
                    }
                    pSession->Start();
                }
                // The client could not connect, so close the pipe. 
                //CloseHandle(hPipe);
            }
		}
        X::EventSystem::I().Unregister("IPC.Session");

	}
    IpcSession::IpcSession(IPC_HANDLE hPipe, IpcServer* srv, void* waitHandle)
    {
        OVERLAPPED* pOverlap = new OVERLAPPED();
        pOverlap->hEvent = waitHandle;
        m_Overlap = pOverlap;
        m_pipe = hPipe;
        m_srv = srv;
        m_waitHandle = waitHandle;
    }
    bool IpcSession::Write(char* data, int size, int& writeSize) 
    {
        DWORD dwWrite = 0;
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = m_waitHandle;
        BOOL bOK = WriteFile(m_pipe,data, size, &dwWrite,
            &overlapped);
        if (!bOK)
        {
            WaitForSingleObject(((OVERLAPPED*)m_Overlap)->hEvent, INFINITE);
            bOK = GetOverlappedResult(m_pipe,
                &overlapped,
                &dwWrite, FALSE);
            bOK = WriteFile(m_pipe, data, size, &dwWrite,
                &overlapped);
        }
        writeSize = (int)dwWrite;
        return bOK;
    }
    bool IpcSession::Send(char* data, int size)
    {
        int cbWrite = 0;
        Write((char*)&size,sizeof(int), cbWrite);
        if (cbWrite != sizeof(int))
        {
            return false;
        }
        Write(data, size, cbWrite);
        return (cbWrite == size);
    }
    void IpcSession::Close()
    {
        m_run = false;
        if (m_pipe)
        {
            //CancelIoEx??
            FlushFileBuffers(m_pipe);
            DisconnectNamedPipe(m_pipe);
            CloseHandle(m_pipe);
            m_pipe = nullptr;
        }
        if (m_waitHandle)
        {
            CloseHandle((HANDLE)m_waitHandle);
            m_waitHandle = nullptr;
        }
    }
    bool IpcSession::Read(char* buf, int bufSize, int& readSize)
    {
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = m_waitHandle;
        DWORD cbBytesRead = 0;
        BOOL fSuccess = ReadFile(
            m_pipe,
            buf,
            bufSize,
            &cbBytesRead,
            nullptr/*&overlapped*/);
        if (!fSuccess)
        {
            DWORD dwErr = GetLastError();
            if (dwErr == ERROR_IO_PENDING)
            {
                fSuccess = GetOverlappedResult(m_pipe,
                    &overlapped,
                    &cbBytesRead, FALSE);
                WaitForSingleObject(((OVERLAPPED*)m_Overlap)->hEvent, INFINITE);
                memset(&overlapped, 0, sizeof(overlapped));
                overlapped.hEvent = m_waitHandle;
                fSuccess = ReadFile(
                    m_pipe,
                    buf,
                    bufSize,
                    &cbBytesRead,
                    &overlapped);
            }
        }
        readSize = (int)cbBytesRead;
        return (fSuccess == TRUE);
    }
    void IpcSession::run()
    {
        X::Event* pEvt = X::EventSystem::I().Query("IPC.Session");
        void* h = pEvt->Add([](void* pContext, X::Event* pEvt) {
            IpcSession* pThis = (IpcSession*)pContext;
            auto valAction = pEvt->Get("action");
            auto strAction = valAction.ToString();
            if (strAction == "end")
            {
                pThis->Close();
            }
            else if (strAction == "notify")
            {
                auto valParam = pEvt->Get("param");
                auto strParam = valParam.ToString();
                std::string notifyInfo = "$notify$" + strParam;
                pThis->Send((char*)notifyInfo.c_str(), notifyInfo.size());
                std::cout << "IPC,Sent out Notify:" << notifyInfo << std::endl;
            }
         },this);
        int bufSize = m_srv->GetBufferSize();
        char* buf = new char[bufSize];
        m_run = true;
        while (m_run)
        {
            int cbBytesRead = 0;
            BOOL fSuccess = FALSE;
            fSuccess = Read(buf, 4, cbBytesRead);
            if (cbBytesRead == 4)
            {
                unsigned int len = *(unsigned int*)buf;
                cbBytesRead = 0;
                fSuccess = Read(buf, len, cbBytesRead);
                if (fSuccess && cbBytesRead == len)
                {
                    buf[cbBytesRead] = 0;
                    if (m_dataHandler)
                    {
                        m_dataHandler(m_HandlerContext, this, buf, len);
                    }
                }
            }
        }
        delete[] buf;
        pEvt->Remove(h);
        pEvt->Release();
        Close();
    }
}