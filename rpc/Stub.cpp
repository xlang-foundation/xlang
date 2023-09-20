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
			ReceiveCall();
			mInsideRecvCall1 = false;
		}
	}
	void XLangStub::run2()
	{
		while (mRun)
		{
			mInsideRecvCall2 = true;
			//ReceiveCall();
			mInsideRecvCall2 = false;
		}
	}
	void XLangStub::ReceiveCall()
	{
		SwapBufferStream stream;
		stream.SetSMSwapBuffer(m_pSwapBuffer1);
		if (!m_pSwapBuffer1->BeginRead() || !mRun)
		{
			return;
		}
		stream.Refresh();
		PayloadFrameHead& head = m_pSwapBuffer1->GetHead();
		bool bOK = false;
		if (head.callType == (unsigned int)RPC_CALL_TYPE::ShakeHands)
		{
			bOK = ShakeHandsCall(head.context,stream);
		}
		else
		{
			RemoteFuncInfo* pFuncInfo = RemotingManager::I().Get(head.callType);
			if (pFuncInfo != nullptr && pFuncInfo->pHandler != nullptr)
			{
				bOK = pFuncInfo->pHandler->Call(head.context,head.callType, stream, this);
			}
			else
			{//wrong call, also need to call lines below
				NotifyBeforeCall(stream);
				NotifyAfterCall(stream, false);
				FinishCall(head.context,stream, false);
			}
		}

	}

	bool XLangStub::ShakeHandsCall(void* pCallContext, SwapBufferStream& stream)
	{
		unsigned long clientPid = 0;
		stream >> clientPid;
		std::cout << "XLangStub::ShakeHandsCall,clientPid=" << clientPid<<std::endl;
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
		FinishCall(pCallContext,stream, true);
		return true;
	}

	void XLangStub::NotifyBeforeCall(SwapBufferStream& stream)
	{
		m_pSwapBuffer1->EndRead();
		//Empty Write to notify another side
		m_pSwapBuffer1->BeginWrite();
		m_pSwapBuffer1->EndWrite();
	}

	void XLangStub::NotifyAfterCall(SwapBufferStream& stream, bool callIsOk)
	{
		m_locker_buffer2.Lock();
		//then we can write to another side
		stream.ReInit();
		stream.SetSMSwapBuffer(m_pSwapBuffer2);
		m_pSwapBuffer2->BeginWrite();
		stream << callIsOk;
	}
	void XLangStub::FinishCall(void* pCallContext,SwapBufferStream& stream,bool callIsOk)
	{
		if (callIsOk)
		{
			PayloadFrameHead& head = m_pSwapBuffer2->GetHead();

			//Pack is End
			//Deliver the last block
			head.payloadType = PayloadType::SendLast;
			head.size = stream.Size();
			//use SwapBuffer is shared memory buffer,
			//we assume it is not too big more then 2G
			//so keep as one block with blockSize
			head.blockSize = (unsigned int)stream.GetPos().offset;
			head.context = pCallContext;
		}
		//write back is finsihed
		m_pSwapBuffer2->EndWrite();//Notify another side
		//empty read to make sure another side read out previous data
		m_pSwapBuffer2->BeginRead();
		m_pSwapBuffer2->EndRead();
		m_locker_buffer2.Unlock();
	}
}