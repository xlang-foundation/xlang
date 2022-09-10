#include "Stub.h"
#include "utility.h"
#include "service_def.h"
#include "sm_buffer.h"
#include "StubMgr.h"
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

	void XLangStub::WatchClientProcess(unsigned long pid)
	{
		std::cout << "WatchClientProcess,pid =" << pid << std::endl;
		m_pStubWatch->SetPid(pid);
		m_pStubWatch->Start();
	}

	void XLangStub::NotifyFromWatch(bool processEnded)
	{
		if (processEnded)
		{
			std::cout << "Client Exited,m_clientPid=" << m_clientPid << std::endl;
			RemotingManager::I().CloseStub(this);
			std::cout << "Stub Closed" << std::endl;
		}
	}

	XLangStub::XLangStub()
	{
		m_pStubWatch = new StubWatch();
		m_pStubWatch->SetParent(this);
		m_pSwapBuffer1 = new SMSwapBuffer();
		m_pSwapBuffer2 = new SMSwapBuffer();
	}
	XLangStub::~XLangStub()
	{
		std::cout << "~XLangStub(),m_clientPid=" << m_clientPid << std::endl;
		delete m_pSwapBuffer1;
		delete m_pSwapBuffer2;
		delete m_pStubWatch;
	}
	bool XLangStub::Create(unsigned long long shmKey)
	{
		mKey = shmKey;
		bool bOK = m_pSwapBuffer1->HostCreate(shmKey, SM_BUF_SIZE);
		if (bOK)
		{
			bOK = m_pSwapBuffer2->HostCreate(shmKey + 1, SM_BUF_SIZE);
		}
		return bOK;
	}


	void XLangStub::Quit()
	{
		mRun = false;

		if (m_pSwapBuffer1)
		{
			m_pSwapBuffer1->ReleaseEvents();
			int cnt = 0;
			while ((cnt < 9999) && mInsideRecvCall1)
			{
				US_SLEEP(33000);
				cnt++;
			}
			m_pSwapBuffer1->Close();
		}
		if (m_pSwapBuffer2)
		{
			m_pSwapBuffer2->ReleaseEvents();
			int cnt = 0;
			while ((cnt < 9999) && mInsideRecvCall2)
			{
				US_SLEEP(33000);
				cnt++;
			}
			m_pSwapBuffer2->Close();
		}
	}

	void XLangStub::run()
	{
		while (mRun)
		{
			mInsideRecvCall1 = true;
			ReceiveCall(1, m_pSwapBuffer1);
			mInsideRecvCall1 = false;
		}
	}
	void XLangStub::run2()
	{
		while (mRun)
		{
			mInsideRecvCall2 = true;
			ReceiveCall(2, m_pSwapBuffer2);
			mInsideRecvCall2 = false;
		}
	}
	void XLangStub::ReceiveCall(int channel, SMSwapBuffer* pSMSwapBuffer)
	{
		SwapBufferStream stream;
		stream.SetSMSwapBuffer(pSMSwapBuffer);
		if (!pSMSwapBuffer->BeginRead() || !mRun)
		{
			return;
		}
		stream.Refresh();
		PayloadFrameHead& head = pSMSwapBuffer->GetHead();
		bool bOK = false;
		if (head.callType == (unsigned int)RPC_CALL_TYPE::ShakeHands)
		{
			bOK = ShakeHandsCall(pSMSwapBuffer, stream);
		}
		else
		{
			RemoteFuncInfo* pFuncInfo = RemotingManager::I().Get(head.callType);
			if (pFuncInfo != nullptr && pFuncInfo->pHandler != nullptr)
			{
				bOK = pFuncInfo->pHandler->Call(channel, head.callType, stream, this);
			}
			else
			{//wrong call, also need to call lines below
				NotifyBeforeCall(channel, stream);
				NotifyAfterCall(channel, stream, false);
			}
		}
		if (bOK)
		{
			//Pack is End
			//Deliver the last block
			head.payloadType = PayloadType::SendLast;
			head.size = stream.Size();
			head.blockSize = stream.GetPos().offset;
		}
		pSMSwapBuffer->EndWrite();//Notify another side
	}

	bool XLangStub::ShakeHandsCall(SMSwapBuffer* pSwapBuffer, SwapBufferStream& stream)
	{
		unsigned long clientPid = 0;
		stream >> clientPid;
		std::cout << "XLangStub::ShakeHandsCall,clientPid=" << clientPid<<std::endl;
		NotifyBeforeCall(1, stream);
		m_clientPid = clientPid;
		if (clientPid != 0)
		{
			WatchClientProcess(clientPid);
		}
		NotifyAfterCall(1, stream, true);
		unsigned long pid = GetPID();
		stream << pid;
		stream << m_sessionId;
		return true;
	}

	void XLangStub::NotifyBeforeCall(int channel, SwapBufferStream& stream)
	{
		(channel == 1 ? m_pSwapBuffer1 : m_pSwapBuffer2)->EndRead();
	}

	void XLangStub::NotifyAfterCall(int channel, SwapBufferStream& stream, bool callIsOk)
	{
		stream.ReInit();
		stream.SetSMSwapBuffer(channel == 1 ? m_pSwapBuffer1 : m_pSwapBuffer2);
		(channel == 1 ? m_pSwapBuffer1 : m_pSwapBuffer2)->BeginWrite();
		stream << callIsOk;
	}
}