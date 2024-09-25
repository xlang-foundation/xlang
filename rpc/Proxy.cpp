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

#include "Proxy.h"
#include "sm_buffer.h"
#include "wait.h"
#include "utility.h"
#include <iostream>
#include "port.h"
#include "manager.h"
#include <string>
#include "ServerCallPool.h"
#include "event.h"
#include "remote_object.h"

#if defined(__APPLE__)
#include <signal.h>
#endif

namespace X
{
	void XLangProxyManager::Register()
	{
		Manager::I().RegisterProxy(LRPC_NAME,[](const char* url) {
			XLangProxy* pProxy = new XLangProxy();
			std::string strUrl(url);
			pProxy->SetUrl(strUrl);
			pProxy->Start();
			return dynamic_cast<XProxy*>(pProxy);
			},
			[](const char* url) {
				return Manager::I().IsLrpcHostedByThisProcess(url);
			});
	}
	XLangProxy::XLangProxy()
	{
		m_pConnectWait = new XWait(false);
		m_pCallReadyWait = new XWait();
		m_pBuffer2ReadyWait = new XWait();
	}
	void XLangProxy::SetUrl(std::string& url)
	{
		SCANF(url.c_str(), "%d", &m_port);
	}
	XLangProxy::~XLangProxy()
	{
		delete m_pConnectWait;
		delete m_pCallReadyWait;
		delete m_pBuffer2ReadyWait;
		for(auto* item: mCallContexts)
		{
			delete item->pWait;
			delete item;
		}
	}
	ROBJ_ID XLangProxy::QueryRootObject(std::string& name)
	{
		if (!CheckConnectReadyStatus())
		{
			return {0,0};
		}
		X::ROBJ_ID oId = {0,0};
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryRootObject,&pContext);
		stream << name;
		auto& stream2 = CommitCall(pContext);
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> oId;
		}
		FinishCall();
		return oId;
	}
	X::Value XLangProxy::UpdateItemValue(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
		Port::vector<std::string>& IdList, int id_offset,
		std::string itemName, X::Value& val)
	{
		if (!CheckConnectReadyStatus())
		{
			return X::Value();
		}
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_UpdateItemValue,&pContext);
		stream << parentObjId;
		stream << id;
		stream << (int)IdList.size();
		for (auto& s : IdList)
		{
			stream << s;
		}
		stream << id_offset;
		stream << itemName;
		stream << val;
		auto& stream2 = CommitCall(pContext);
		X::Value retVal;
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> retVal;
		}
		FinishCall();
		return retVal;
	}
	bool XLangProxy::FlatPack(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
		Port::vector<std::string>& IdList, int id_offset,
		long long startIndex, long long count, Value& retList)
	{
		if (!CheckConnectReadyStatus())
		{
			return false;
		}
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_FlatPack,&pContext);
		stream << parentObjId;
		stream << id;
		stream << (int)IdList.size();
		for (auto& s : IdList)
		{
			stream << s;
		}
		stream << id_offset;
		stream << startIndex;
		stream << count;
		auto& stream2 = CommitCall(pContext);
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> retList;
		}
		FinishCall();
		return true;
	}
	X::ROBJ_MEMBER_ID XLangProxy::QueryMember(X::ROBJ_ID id, std::string& name,
		int& memberFlags)
	{
		if (!CheckConnectReadyStatus())
		{
			return -1;
		}
		X::ROBJ_MEMBER_ID mId = -1;
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember,&pContext);
		stream << id;
		stream << name;
		auto& stream2 = CommitCall(pContext);
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			stream2 >> mId;
			stream2 >> memberFlags;
		}
		FinishCall();
		return mId;
	}
	long long XLangProxy::QueryMemberCount(X::ROBJ_ID id)
	{
		if (!CheckConnectReadyStatus())
		{
			return -1;
		}
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMemberCount,&pContext);
		stream << id;
		auto& stream2 = CommitCall(pContext);
		bool bOK = false;
		stream2 >> bOK;
		long long cnt = -1;
		if (bOK)
		{
			stream2 >> cnt;
		}
		FinishCall();
		return cnt;
	}
	bool XLangProxy::ReleaseObject(ROBJ_ID id)
	{
		if (!CheckConnectReadyStatus())
		{
			return false;
		}
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_ReleaseObject,&pContext);
		stream << id;
		auto& stream2 = CommitCall(pContext);
		bool bOK = false;
		stream2 >> bOK;
		FinishCall();
		return bOK;
	}
	X::ROBJ_ID XLangProxy::GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId)
	{
		if (!CheckConnectReadyStatus())
		{
			return {0,0};
		}
		X::ROBJ_ID oId = {0,0};
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject,&pContext);
		stream << objid;
		stream << memId;
		auto& stream2 = CommitCall(pContext);
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
		X::ROBJ_ID oId = {0,0};
		Call_Context* pCallContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_Call,&pCallContext);
		stream.ScopeSpace().SetContext((XlangRuntime*)rt, pContext);

		stream << parent_id;
		stream << id;
		stream << memId;

		int argNum = (int)params.size();
		stream << argNum;
		//Pack Parameters
		for (auto& param : params)
		{
			bool bNeedConvert = false;
			if (param.IsObject())
			{
				// for function as an event handler, we need to convert it to remote client object
				//converT to remote client object
				auto* pObj = param.GetObj();
				auto type = pObj->GetType();
				if ( type== X::ObjType::Function || type == X::ObjType::PyProxyObject)
				{
					bNeedConvert = true;
				}
			}
			if (bNeedConvert)
			{
				auto&& rcParam = ConvertXObjToRemoteClientObject(param.GetObj());
				rcParam.ToBytes(&stream);
			}
			else
			{
				param.ToBytes(&stream);
			}
		}
		stream << (int)kwParams.size();
		for (auto& kw : kwParams)
		{
			stream << kw.key;
			bool bNeedConvert = false;
			if (kw.val.IsObject())
			{
				// for function as an event handler, we need to convert 
				// it to remote client object
				//converT to remote client object
				auto* pObj = kw.val.GetObj();
				auto type = pObj->GetType();
				if (type == X::ObjType::Function || type == X::ObjType::PyProxyObject)
				{
					bNeedConvert = true;
				}
			}
			if (bNeedConvert)
			{
				//convert to remote client object
				auto&& rcParam = ConvertXObjToRemoteClientObject(kw.val.GetObj());
				rcParam.ToBytes(&stream);
			}
			else
			{
				kw.val.ToBytes(&stream);
			}
		}
		//set flag to show if there is a trailer
		stream << trailer.IsValid();
		if (trailer.IsValid())
		{
			stream << trailer;
		}
		//std::cout << "Before CommitCall" << std::endl;
		auto& stream2 = CommitCall(pCallContext);
		bool bOK = false;
		stream2 >> bOK;
		if (bOK)
		{
			X::ROBJ_ID retId = {0,0};
			stream2 >> retId;
			if (retId.objId ==0)
			{//value
				retValue.FromBytes(&stream2);
			}
			else
			{
				X::XRemoteObject* pRetObj =
					X::g_pXHost->CreateRemoteObject(this);
				pRetObj->SetObjID((unsigned long)retId.pid,retId.objId);
				retValue = (X::XObj*)pRetObj;
				pRetObj->DecRef();
			}
		}
		//std::cout << "After CommitCall and Before FinishCall" << std::endl;
		FinishCall();
		//std::cout << "After FinishCall" << std::endl;
		return bOK;
	}

	SwapBufferStream& XLangProxy::BeginCall(unsigned int callType, Call_Context** ppContext)
	{
		mCallLock1.Lock();
		mStream1.ReInit();
		mSMSwapBuffer1->BeginWrite();

		mStream1.SetSMSwapBuffer(mSMSwapBuffer1);
		Call_Context* pContext = GetCallContext();
		PayloadFrameHead& head = mSMSwapBuffer1->GetHead();
		head.payloadType = PayloadType::Send;
		head.size = 0;//update later
		head.callType = callType;
		head.callIndex = 0;
		head.context = pContext;
		*ppContext = pContext;

		return mStream1;
	}

	SwapBufferStream& XLangProxy::CommitCall(Call_Context* pContext)
	{
		PayloadFrameHead& head = mSMSwapBuffer1->GetHead();
		//Deliver the last block
		head.payloadType = PayloadType::SendLast;
		head.size = mStream1.Size();
		//use SwapBuffer is shared memory buffer,
		//we assume it is not too big more then 2G
		//so keep as one block with blockSize
		head.blockSize = (unsigned int)mStream1.GetPos().offset;
		mSMSwapBuffer1->EndWrite();//Notify another side
		//read to make sure another side has read the data
		mSMSwapBuffer1->BeginRead();
		mSMSwapBuffer1->EndRead();
		mCallLock1.Unlock();//unlock to let other call come in

		//Wait for pContext->Ready
		//std::cout << "In CommitCall before wait" << std::endl;
		pContext->pWait->Wait(mTimeout);
		//std::cout << "In CommitCall after wait" << std::endl;
		ReturnCallContext(pContext);
		//Fetch Result
		mStream2.ReInit();
		mStream2.SetSMSwapBuffer(mSMSwapBuffer2);
		mStream2.Refresh();

		return mStream2;
	}

	void XLangProxy::FinishCall()
	{
		m_pCallReadyWait->Release();
	}
	//use process as signal to host exit
	//this way will not work for case host is in Admin or service mode
	//so we need to use semaphore, in fact, in windows, we use event
	void XLangProxy::WaitToHostExit()
	{
		//for wait to host exit
		SEMAPHORE_HANDLE semaphore = nullptr;

		auto ret = 1;
		while (ret != 0)
		{
			std::string semaphoreName =
				mHostUseGlobal ? "Global\\XlangServerSemaphore_" : "XlangServerSemaphore_";
			semaphoreName += std::to_string(mHostProcessId);
			semaphore = OPEN_SEMAPHORE(semaphoreName.c_str());
			if (semaphore == nullptr)
			{
				std::cout << "Host semaphore" << semaphoreName << "is gone,host exited" << std::endl;
				break;
			}
			ret = WAIT_FOR_SEMAPHORE(semaphore, 30);
			CLOSE_SEMAPHORE(semaphore);
		}
	}
	void XLangProxy::WaitToHostExit_ByProcess()
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
		AddRef();
		ThreadAddRef();
		while (mRun)
		{
			bool bOK = Connect();
			if (!bOK)
			{
				std::cout << "Client:Tried with fail, will try again" << std::endl;
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
				m_ConnectLock.Lock();
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
				m_bConnected = false;
				m_ConnectLock.Unlock();
				break;
			}
			else
			{	
				//for multiple re-entru, need to reset
				//but this is not correct way, need to fix
				//TODO: fix this
				m_pConnectWait->Reset();
			}
			m_ConnectLock.Lock();
			if (mSMSwapBuffer1)
			{
				mSMSwapBuffer1->Close();
			}
			if (mSMSwapBuffer2)
			{
				mSMSwapBuffer2->Close();
			}
			m_bConnected = false;
			m_ConnectLock.Unlock();
		}
		//cal this one before Release(),to avoid this pointer deleted by Release()
		ThreadRelease();
		Release();
	}
	void XLangProxy::run2()
	{
		AddRef();
		ThreadAddRef();
		bool bWaitOnBuffer2 = true;
		while (mRun)
		{
			if (bWaitOnBuffer2)
			{
				m_pBuffer2ReadyWait->Wait(mTimeout);
				bWaitOnBuffer2 = false;
			}
			if (!mSMSwapBuffer2->BeginRead())
			{
				continue;
			}
			//std::cout << "Before wait" <<std::endl;
			PayloadFrameHead& head = mSMSwapBuffer2->GetHead();
			if (head.context)
			{
				Call_Context* pContext = (Call_Context*)head.context;
				pContext->pWait->Release();
				//wait for call read out its return data
				m_pCallReadyWait->Wait(mTimeout);
				mSMSwapBuffer2->EndRead();
			}
			else
			{//notify from server side, no wait, we don't support return valur for this call
				mStream2.ReInit();
				mStream2.SetSMSwapBuffer(mSMSwapBuffer2);
				mStream2.Refresh();
				ROBJ_ID clientObjId;
				mStream2 >> clientObjId;
				int argNum;
				mStream2 >> argNum;
				ARGS params(argNum);
				KWARGS kwParams;
				for (int i = 0; i < argNum; i++)
				{
					Value val;
					val.FromBytes(&mStream2);
					params.push_back(val);
				}
				int kwNum;
				mStream2 >> kwNum;
				for (int i = 0; i < kwNum; i++)
				{
					std::string key;
					mStream2 >> key;
					Value val;
					val.FromBytes(&mStream2);
					kwParams.Add(key.c_str(),val);
				}
				mSMSwapBuffer2->EndRead();

				auto* pClientObj = CovertIdToXObj(clientObjId);
				if(pClientObj)
				{
					SrvCallInfo srvInfo{ pClientObj,params, kwParams };
					X::ServerCallPool::I().AddCall(srvInfo);
					//X::Value retValue;
					//pClientObj->Call(nullptr, nullptr, params,kwParams,retValue);
				}
			}
			//do an empty write to notify server side can write again
			mSMSwapBuffer2->BeginWrite();
			mSMSwapBuffer2->EndWrite();
		}
		//cal this one before Release(),to avoid this pointer deleted by Release()
		ThreadRelease();
		Release();
	}
	void XLangProxy::Cleanup()
	{
		m_ConnectLock.Lock();
		m_bConnected = false;
		m_ConnectLock.Unlock();
		Manager::I().RemoveProxy(LRPC_NAME,mRootObjectName,this);
	}
	Call_Context* XLangProxy::GetCallContext()
	{
		Call_Context* pContext = nullptr;
		m_CallContextLock.Lock();
		for(auto* item: mCallContexts)
		{
			if (!item->InUse)
			{
				pContext = item;
				break;
			}
		}
		if (pContext == nullptr)
		{
			pContext = new Call_Context();
			mCallContexts.push_back(pContext);
			pContext->pWait = new XWait();
		}
		pContext->InUse = true;
		m_CallContextLock.Unlock();
		return pContext;
	}
	bool XLangProxy::CheckConnectReadyStatus()
	{
		bool bReady = false;
		m_ConnectLock.Lock();
		bReady = m_bConnected;
		m_ConnectLock.Unlock();
		if (!bReady)
		{
			if (m_pConnectWait->Wait(mTimeout))
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

		unsigned long pid = GetPID();
		unsigned long long shmKey = rand64();
		shmKey <<= 32;
		shmKey |= pid;

		mSMSwapBuffer1 = new SMSwapBuffer();
		mSMSwapBuffer2 = new SMSwapBuffer();

		bool bOK = mSMSwapBuffer1->ClientConnect(mHostUseGlobal,m_port,shmKey,SM_BUF_SIZE, timeoutMS);
		if (bOK)
		{
			bOK = mSMSwapBuffer2->ClientConnect(mHostUseGlobal,m_port, shmKey+1,SM_BUF_SIZE, timeoutMS, false);
			if (bOK)
			{
				m_pBuffer2ReadyWait->Release();
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
		Call_Context* pContext = nullptr;
		auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::ShakeHands,&pContext);
		stream << pid_client;
		auto& stream2 = CommitCall(pContext);
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
	void XLangProxy::AddObject(XObj* obj)
	{
	}
	void XLangProxy::RemoveOject(XObj* obj)
	{
	}
}