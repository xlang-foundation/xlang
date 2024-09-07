#include "RemotingServer.h"
#include "utility.h"
#include "service_def.h"
#include "SMSwapBuffer.h"

#include "port.h"
#include <iostream>

#if (!WIN32)
#include <signal.h>
#endif

namespace X
{
	void StubWatch::run()
	{
		while (mRun)
		{
			WaitToHostExit();
			mParent->NotifyFromWatch(true);
			break;
		}
	}

	void StubWatch::WaitToHostExit()
	{
#if (WIN32)
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, mPidToWatch);
		::WaitForSingleObject(hProcess, INFINITE);
		CloseHandle(hProcess);
#else
		//waitpid only works for child-process
		//so have to use kill with 0 pass
		//https://unix.stackexchange.com/questions/214908/why-can-the-waitpid-system-call-only-be-used-with-child-processes
		int res = kill((pid_t)mPidToWatch, 0);
		while (res == 0 || (res < 0 && errno == EPERM))
		{
			sleep(1);
			res = kill((pid_t)mPidToWatch, 0);
		}
#endif
		mPidToWatch = 0;
	}

	void RemotingServer::WatchClientProcess(unsigned long pid)
	{
		std::cout << "WatchClientProcess,pid =" << pid << std::endl;
		m_pStubWatch->SetPid(pid);
		m_pStubWatch->Start();
	}

	void RemotingServer::NotifyFromWatch(bool processEnded)
	{
		if (processEnded)
		{
			std::cout << "Client Exited,m_clientPid=" << m_clientPid << std::endl;
			RemotingManager::I().CloseStub(this);
			std::cout << "Stub Closed" << std::endl;
		}
	}

	RemotingServer::RemotingServer()
	{
		m_pStubWatch = new StubWatch();
		m_pStubWatch->SetParent(this);
		mWBuffer = new SMSwapBuffer();
		mRBuffer = new SMSwapBuffer();
	}
	RemotingServer::~RemotingServer()
	{
		std::cout << "~RemotingServer(),m_clientPid=" << m_clientPid << std::endl;
		delete mWBuffer;
		delete mRBuffer;
		delete m_pStubWatch;
	}
	bool RemotingServer::Create(unsigned long long shmKey)
	{
		mKey = shmKey;
		bool bOK = mWBuffer->HostCreate(shmKey, SM_BUF_SIZE);
		if (bOK)
		{
			bOK = mRBuffer->HostCreate(shmKey + 1, SM_BUF_SIZE);
		}
		return bOK;
	}


	void RemotingServer::Quit()
	{
		mRun = false;

		if (mWBuffer)
		{
			mWBuffer->ReleaseEvents();
			int cnt = 0;
			while ((cnt < 9999) && mInsideRecvCall1)
			{
				US_SLEEP(33000);
				cnt++;
			}
			mWBuffer->Close();
		}
		if (mRBuffer)
		{
			mRBuffer->ReleaseEvents();
			int cnt = 0;
			while ((cnt < 9999) && mInsideRecvCall2)
			{
				US_SLEEP(33000);
				cnt++;
			}
			mRBuffer->Close();
		}
	}

	void RemotingServer::run()
	{
		while (mRun)
		{

		}
	}

	void RemotingServer::ReceiveCall()
	{
		SwapBufferStream stream;
		stream.SetSMSwapBuffer(mWBuffer);
		if (!mWBuffer->BeginRead() || !mRun)
		{
			return;
		}
		stream.Refresh();
		PayloadFrameHead& head = mWBuffer->GetHead();
		bool bOK = false;
		if (head.callType == (unsigned int)RPC_CALL_TYPE::ShakeHands)
		{
			bOK = ShakeHandsCall(head.context, stream);
		}
		else
		{
			RemoteFuncInfo* pFuncInfo = RemotingManager::I().Get(head.callType);
			if (pFuncInfo != nullptr && pFuncInfo->pHandler != nullptr)
			{
				bOK = pFuncInfo->pHandler->Call(head.context, head.callType, stream, this);
			}
			else
			{//wrong call, also need to call lines below
				NotifyBeforeCall(stream);
				NotifyAfterCall(stream, false);
				FinishCall(head.context, stream, false);
			}
		}

	}

	bool RemotingServer::ShakeHandsCall(void* pCallContext, SwapBufferStream& stream)
	{
		unsigned long clientPid = 0;
		stream >> clientPid;
		std::cout << "RemotingServer::ShakeHandsCall,clientPid=" << clientPid << std::endl;
		NotifyBeforeCall(stream);
		m_clientPid = clientPid;
		if (clientPid != 0)
		{
			WatchClientProcess(clientPid);
		}
		NotifyAfterCall(stream, true);
		unsigned long pid = GetPID();
		stream << pid;
		stream << m_sessionId;
		FinishCall(pCallContext, stream, true);
		return true;
	}
}