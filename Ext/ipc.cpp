#include "ipc.h"

#include <iostream>
#include "event.h"

#if (!WIN32)
namespace X
{
	void IpcServer::run()
	{
	}
    IpcSession::IpcSession(IPC_HANDLE hPipe, IpcServer* srv, void* waitHandle)
    {
    }
    bool IpcSession::Write(char* data, int size, int& writeSize) 
    {
        return false;
    }
    bool IpcSession::Send(char* data, int size)
    {
        return false;
    }
    void IpcSession::Close()
    {
    }
    bool IpcSession::Read(char* buf, int bufSize, int& readSize)
    {
        return false;
    }
    void IpcSession::run()
    {
    }
}
#endif