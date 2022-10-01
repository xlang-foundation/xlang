#include "Proxy.h"
#include "sm_buffer.h"
#include "wait.h"
#include "utility.h"
#include <time.h>
#include <iostream>
#include "port.h"
#include "manager.h"
#include <string>

namespace X
{
	void XLangProxyManager::Register()
	{
		Manager::I().RegisterProxy("lrpc",[](std::string& url) {
			XLangProxy* pProxy = new XLangProxy();
			pProxy->Start();
			return dynamic_cast<XProxy*>(pProxy);
			});
	}
	XLangProxy::XLangProxy()
	{
		m_pConnectWait = new XWait(false);
	}

	XLangProxy::~XLangProxy()
	{
		delete m_pConnectWait;
	}
	ROBJ_ID XLangProxy::QueryRootObject(std::string& name)
	{
		if (!CheckConnectReadyStatus())
		{
			return nullptr;
		}
		X::ROBJ_ID oId = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryRootObject);
		stream << name;
		auto& stream2 = CommitCall();
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> oId;
		}
		FinishCall();
		return oId;
	}
	X::ROBJ_MEMBER_ID XLangProxy::QueryMember(X::ROBJ_ID id, std::string& name,
		bool& KeepRawParams)
	{
		if (!CheckConnectReadyStatus())
		{
			return -1;
		}
		X::ROBJ_MEMBER_ID mId = -1;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember);
		stream << id;
		stream << name;
		auto& stream2 = CommitCall();
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> mId;
			stream2 >> KeepRawParams;
		}
		FinishCall();
		return mId;
	}

	X::ROBJ_ID XLangProxy::GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId)
	{
		if (!CheckConnectReadyStatus())
		{
			return nullptr;
		}
		X::ROBJ_ID oId = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject);
		stream << objid;
		stream << memId;
		auto& stream2 = CommitCall();
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> oId;
		}
		FinishCall();
		return oId;
	}

	bool XLangProxy::Call(XRuntime* rt, XObj* pContext,
		X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
		X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer,X::Value& retValue)
	{
		if (!CheckConnectReadyStatus())
		{
			return false;
		}
		X::ROBJ_ID oId = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_Call);
		stream.ScopeSpace().SetContext((Runtime*)rt, pContext);

		stream << parent_id;
		stream << id;
		stream << memId;

		int argNum = (int)params.size();
		stream << argNum;
		//Pack Parameters
		for (auto& param : params)
		{
			param.ToBytes(&stream);
		}
		stream << (int)kwParams.size();
		for (auto& kw : kwParams)
		{
			stream << kw.first;
			kw.second.ToBytes(&stream);
		}
		//set flag to show if there is a trailer
		stream << trailer.IsValid();
		if (trailer.IsValid())
		{
			stream << trailer;
		}

		auto& stream2 = CommitCall();
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			X::ROBJ_ID retId = 0;
			stream2 >> retId;
			if (retId == 0)
			{//value
				retValue.FromBytes(&stream2);
			}
			else
			{
				X::XRemoteObject* pRetObj =
					X::g_pXHost->CreateRemoteObject(this);
				pRetObj->SetObjID(retId);
				retValue = (X::XObj*)pRetObj;
			}
		}
		FinishCall();
		return bOK;
	}

	SwapBufferStream& XLangProxy::BeginCall(unsigned int callType)
	{
		mCallLock1.Lock();
		mStream1.ReInit();
		mSMSwapBuffer1->BeginWrite();

		mStream1.SetSMSwapBuffer(mSMSwapBuffer1);
		PayloadFrameHead& head = mSMSwapBuffer1->GetHead();
		head.payloadType = PayloadType::Send;
		head.size = 0;//update later
		head.callType = callType;
		head.memberIndex = 0;
		head.context = nullptr;

		return mStream1;
	}

	SwapBufferStream& XLangProxy::CommitCall()
	{
		PayloadFrameHead& head = mSMSwapBuffer1->GetHead();
		//Deliver the last block
		head.payloadType = PayloadType::SendLast;
		head.size = mStream1.Size();
		head.blockSize = mStream1.GetPos().offset;
		mSMSwapBuffer1->EndWrite();//Notify another side
		//Fetch Result
		mStream1.ReInit();
		mStream1.SetSMSwapBuffer(mSMSwapBuffer1);
		if (!mSMSwapBuffer1->BeginRead())//wait for results
		{
			throw XLangStreamException(-100);
		}
		mStream1.Refresh();

		return mStream1;
	}

	void XLangProxy::FinishCall()
	{
		mSMSwapBuffer1->EndRead();
		mCallLock1.Unlock();
	}
	SwapBufferStream& XLangProxy::BeginCall2(unsigned int callType)
	{
		mCallLock2.Lock();
		mStream2.ReInit();
		mSMSwapBuffer2->BeginWrite();

		mStream2.SetSMSwapBuffer(mSMSwapBuffer2);
		PayloadFrameHead& head = mSMSwapBuffer2->GetHead();
		head.payloadType = PayloadType::Send;
		head.size = 0;//update later
		head.callType = callType;
		head.memberIndex = 0;
		head.context = nullptr;

		return mStream2;
	}

	SwapBufferStream& XLangProxy::CommitCall2()
	{
		PayloadFrameHead& head = mSMSwapBuffer2->GetHead();
		//Deliver the last block
		head.payloadType = PayloadType::SendLast;
		head.size = mStream2.Size();
		head.blockSize = mStream2.GetPos().offset;
		mSMSwapBuffer2->EndWrite();//Notify another side
		//Fetch Result
		mStream2.ReInit();
		mStream2.SetSMSwapBuffer(mSMSwapBuffer2);
		if (!mSMSwapBuffer2->BeginRead())//wait for results
		{
			throw XLangStreamException(-100);
		}
		mStream2.Refresh();

		return mStream2;
	}

	void XLangProxy::FinishCall2()
	{
		mSMSwapBuffer2->EndRead();
		mCallLock2.Unlock();
	}
	void XLangProxy::WaitToHostExit()
	{
#if (WIN32)
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, mHostProcessId);
		::WaitForSingleObject(hProcess, INFINITE);
		CloseHandle(hProcess);
#else
		//waitpid only works for child-process
		//so have to use kill with 0 pass
		//https://unix.stackexchange.com/questions/214908/why-can-the-waitpid-system-call-only-be-used-with-child-processes
		int res = kill((pid_t)mHostProcessId, 0);
		while (res == 0 || (res < 0 && errno == EPERM))
		{
			sleep(1);
			res = kill((pid_t)mHostProcessId, 0);
		}
#endif
		mHostProcessId = 0;
	}
	void XLangProxy::run()
	{
		while (mRun)
		{
			bool bOK = Connect();
			if (!bOK)
			{
				std::cout << "CantorClient:Tried with fail, will try again"<<std::endl;
				MS_SLEEP(200);
				continue;
			}
			m_ConnectLock.Lock();
			m_bConnected = true;
			m_ConnectLock.Unlock();
			m_pConnectWait->Release();
			WaitToHostExit();
			if (m_ExitOnHostExit)
			{
				mRun = false;
				m_Exited = true;
				if (mSMSwapBuffer1)
				{
					mSMSwapBuffer1->Close();
				}
				if (mSMSwapBuffer2)
				{
					mSMSwapBuffer2->Close();
				}
				break;
			}
			m_ConnectLock.Lock();
			m_bConnected = false;
			m_ConnectLock.Unlock();
		}
	}
	void XLangProxy::run2()
	{
	}
	bool XLangProxy::CheckConnectReadyStatus()
	{
		bool bReady = false;
		m_ConnectLock.Lock();
		bReady = m_bConnected;
		m_ConnectLock.Unlock();
		if (!bReady)
		{
			if (m_pConnectWait->Wait(-1))
			{
				m_ConnectLock.Lock();
				bReady = m_bConnected;
				m_ConnectLock.Unlock();
			}
		}
		return bReady;
	}
	bool XLangProxy::Connect()
	{
		int timeoutMS = -1;

		srand(time(nullptr));
		unsigned long pid = GetPID();
		unsigned long long shmKey = rand();
		shmKey <<= 32;
		shmKey |= pid;

		mSMSwapBuffer1 = new SMSwapBuffer();
		mSMSwapBuffer2 = new SMSwapBuffer();

		bool bOK = mSMSwapBuffer1->ClientConnect(shmKey, SM_BUF_SIZE, timeoutMS);
		if (bOK)
		{
			bOK = mSMSwapBuffer2->ClientConnect(shmKey + 1, SM_BUF_SIZE, timeoutMS, false);
			if (bOK)
			{
				ShapeHandsToServer();
			}
		}
		return bOK;
	}

	void XLangProxy::Disconnect()
	{
	}
	void XLangProxy::ShapeHandsToServer()
	{
		unsigned long pid_client = GetPID();
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::ShakeHands);
		stream << pid_client;
		auto& stream2 = CommitCall();
		bool bOK = false;
		unsigned long pid_srv = 0;
		unsigned long long sid = 0;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> pid_srv;
			stream2 >> sid;
		}
		FinishCall();
		mHostProcessId = pid_srv;
		mSessionId = sid;
	}
}