#include "RemotingProxy.h"
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
	namespace IPC
	{
		void RemotingProxyManager::Register()
		{
			Manager::I().RegisterProxy(LRPC_NAME, [](const char* url) {
				RemotingProxy* pProxy = new RemotingProxy();
				std::string strUrl(url);
				pProxy->SetUrl(strUrl);
				pProxy->Start();
				return dynamic_cast<XProxy*>(pProxy);
				},
				[](const char* url) {
					return Manager::I().IsLrpcHostedByThisProcess(url);
				});
		}
		RemotingProxy::RemotingProxy()
		{
			mMinReqId = 1;
			mMaxReqId = 10000;
			mNextRequestId = mMinReqId;
			m_pConnectWait = new XWait(false);
		}
		void RemotingProxy::SetUrl(std::string& url)
		{
			SCANF(url.c_str(), "%d", &m_port);
		}
		RemotingProxy::~RemotingProxy()
		{
			delete m_pConnectWait;
		}
		ROBJ_ID RemotingProxy::QueryRootObject(std::string& name)
		{
			if (!CheckConnectReadyStatus())
			{
				return { 0,0 };
			}
			X::ROBJ_ID oId = { 0,0 };
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryRootObject, context);
			stream << name;
			auto& stream2 = CommitCall(context);
			bool bOK = false;
			stream2 >> bOK;
			if (bOK)
			{
				stream2 >> oId;
			}
			FinishCall();
			return oId;
		}
		X::Value RemotingProxy::UpdateItemValue(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			std::string itemName, X::Value& val)
		{
			if (!CheckConnectReadyStatus())
			{
				return X::Value();
			}
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_UpdateItemValue, context);
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
			auto& stream2 = CommitCall(context);
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
		bool RemotingProxy::FlatPack(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count, Value& retList)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_FlatPack, context);
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
			auto& stream2 = CommitCall(context);
			bool bOK = false;
			stream2 >> bOK;
			if (bOK)
			{
				stream2 >> retList;
			}
			FinishCall();
			return true;
		}
		X::ROBJ_MEMBER_ID RemotingProxy::QueryMember(X::ROBJ_ID id, std::string& name,
			int& memberFlags)
		{
			if (!CheckConnectReadyStatus())
			{
				return -1;
			}
			X::ROBJ_MEMBER_ID mId = -1;
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMember, context);
			stream << id;
			stream << name;
			auto& stream2 = CommitCall(context);
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
		long long RemotingProxy::QueryMemberCount(X::ROBJ_ID id)
		{
			if (!CheckConnectReadyStatus())
			{
				return -1;
			}
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_QueryMemberCount, context);
			stream << id;
			auto& stream2 = CommitCall(context);
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
		bool RemotingProxy::ReleaseObject(ROBJ_ID id)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_ReleaseObject, context);
			stream << id;
			auto& stream2 = CommitCall(context);
			bool bOK = false;
			stream2 >> bOK;
			FinishCall();
			return bOK;
		}
		X::ROBJ_ID RemotingProxy::GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId)
		{
			if (!CheckConnectReadyStatus())
			{
				return { 0,0 };
			}
			X::ROBJ_ID oId = { 0,0 };
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_GetMemberObject, context);
			stream << objid;
			stream << memId;
			auto& stream2 = CommitCall(context);
			bool bOK = false;
			stream2 >> bOK;
			if (bOK)
			{
				stream2 >> oId;
			}
			FinishCall();
			return oId;
		}

		bool RemotingProxy::Call(XRuntime* rt, XObj* pContext,
			X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			X::ROBJ_ID oId = { 0,0 };
			Call_Context callContext;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::CantorProxy_Call, callContext);
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
					if (type == X::ObjType::Function || type == X::ObjType::PyProxyObject)
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
					//convert to remote client object
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
			auto& stream2 = CommitCall(callContext);
			bool bOK = false;
			stream2 >> bOK;
			if (bOK)
			{
				X::ROBJ_ID retId = { 0,0 };
				stream2 >> retId;
				if (retId.objId == 0)
				{//value
					retValue.FromBytes(&stream2);
				}
				else
				{
					X::XRemoteObject* pRetObj =
						X::g_pXHost->CreateRemoteObject(this);
					pRetObj->SetObjID((unsigned long)retId.pid, retId.objId);
					retValue = (X::XObj*)pRetObj;
					pRetObj->DecRef();
				}
			}
			//std::cout << "After CommitCall and Before FinishCall" << std::endl;
			FinishCall();
			//std::cout << "After FinishCall" << std::endl;
			return bOK;
		}

		//use process as signal to host exit
		//this way will not work for case host is in Admin or service mode
		//so we need to use semaphore, in fact, in windows, we use event
		void RemotingProxy::WaitToHostExit()
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
		void RemotingProxy::WaitToHostExit_ByProcess()
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
		void RemotingProxy::run()
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
					m_bConnected = false;
					m_ConnectLock.Unlock();
					break;
				}
				else
				{
					//for multiple re-entry, need to reset
					//but this is not correct way, need to fix
					//TODO: fix this
					m_pConnectWait->Reset();
				}
				m_ConnectLock.Lock();
				m_bConnected = false;
				m_ConnectLock.Unlock();
			}
			//cal this one before Release(),to avoid this pointer deleted by Release()
			ThreadRelease();
			Release();
		}
		void RemotingProxy::Cleanup()
		{
			m_ConnectLock.Lock();
			m_bConnected = false;
			m_ConnectLock.Unlock();
			Manager::I().RemoveProxy(LRPC_NAME, mRootObjectName, this);
		}

		bool RemotingProxy::CheckConnectReadyStatus()
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
		bool RemotingProxy::Connect()
		{
			int timeoutMS = -1;

			unsigned long pid = GetPID();
			unsigned long long shmKey = rand64();
			shmKey <<= 32;
			shmKey |= pid;

			mWBuffer = new SMSwapBuffer();
			mRBuffer = new SMSwapBuffer();
			//server side use shmKey as WriteBuffer and shmKey+1 as ReadBuffer
			//so client side need to switch them
			bool bOK = mRBuffer->ClientConnect(mHostUseGlobal, m_port, shmKey, SM_BUF_SIZE, timeoutMS);
			if (bOK)
			{
				bOK = mWBuffer->ClientConnect(mHostUseGlobal, m_port, shmKey + 1, SM_BUF_SIZE, timeoutMS, false);
				if (bOK)
				{
					StartReadThread();
					ShapeHandsToServer();
				}
			}
			return bOK;
		}

		void RemotingProxy::Disconnect()
		{
		}
		void RemotingProxy::ShapeHandsToServer()
		{
			unsigned long pid_client = GetPID();
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::ShakeHands, context);
			stream << pid_client;
			auto& stream2 = CommitCall(context);
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
	}//namespace IPC
}//namespace X