#pragma once
#include "gthread.h"
#include <string>
namespace X
{
	typedef void* IPC_HANDLE;
	class IpcSession;
	typedef void (*On_NewSession)(void*,IpcSession*);
	typedef void (*On_Data)(void*,IpcSession*,char* data,int size);
	class IpcServer:
		public GThread
	{
		std::string m_path;
		int m_bufferSize = 512;
		void* m_pHandlerContext = nullptr;
		On_NewSession m_newSessionHandler =nullptr;
	public:
		IpcServer(const char* path,void* pContext,
			On_NewSession newSessionHandler)
		{
			m_pHandlerContext = pContext;
			m_newSessionHandler = newSessionHandler;
			m_path = path;
		}
		IpcServer(std::string& path, 
			On_NewSession newSessionHandler)
		{
			m_newSessionHandler = newSessionHandler;
			m_path = path;
		}
		void SetBufferSize(int s)
		{
			m_bufferSize = s;
		}
		int GetBufferSize()
		{
			return m_bufferSize;
		}
		// Inherited via GThread
		virtual void run() override;
	};
	class IpcSession :
		public GThread
	{
		IpcServer* m_srv = nullptr;
		IPC_HANDLE m_pipe = nullptr;
		void* m_HandlerContext = nullptr;
		On_Data  m_dataHandler = nullptr;
	public:
		IpcSession(IPC_HANDLE hPipe, IpcServer* srv)
		{
			m_pipe = hPipe;
			m_srv = srv;
		}
		void SetDataHandler(void* pContext,On_Data dataHandler)
		{
			m_HandlerContext = pContext;
			m_dataHandler = dataHandler;
		}
		bool Send(char* data, int size);
		// Inherited via GThread
		virtual void run() override;
	};
}