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
				pProxy->StartProxy();
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
			StopRunning();
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
			long long returnCode = 0;
			auto& stream2 = CommitCall(context, returnCode);
			if (returnCode>0)
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
			return CallHandler::UpdateItemValue(parentObjId, id, IdList, id_offset, itemName, val);
		}
		bool RemotingProxy::FlatPack(X::ROBJ_ID parentObjId, X::ROBJ_ID id,
			Port::vector<std::string>& IdList, int id_offset,
			long long startIndex, long long count, Value& retList)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			return CallHandler::FlatPack(parentObjId, id, IdList, 
				id_offset, startIndex, count, retList);
		}
		X::ROBJ_MEMBER_ID RemotingProxy::QueryMember(X::ROBJ_ID id, std::string& name,
			int& memberFlags)
		{
			if (!CheckConnectReadyStatus())
			{
				return -1;
			}
			return CallHandler::QueryMember(id, name, memberFlags);
		}
		long long RemotingProxy::QueryMemberCount(X::ROBJ_ID id)
		{
			if (!CheckConnectReadyStatus())
			{
				return -1;
			}
			return CallHandler::QueryMemberCount(id);
		}
		bool RemotingProxy::ReleaseObject(ROBJ_ID id)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			return CallHandler::ReleaseObject(id);
		}
		X::ROBJ_ID RemotingProxy::GetMemberObject(X::ROBJ_ID objid, X::ROBJ_MEMBER_ID memId)
		{
			if (!CheckConnectReadyStatus())
			{
				return { 0,0 };
			}
			return CallHandler::GetMemberObject(objid, memId);
		}

		bool RemotingProxy::Call(XRuntime* rt, XObj* pContext,
			X::ROBJ_ID parent_id, X::ROBJ_ID id, X::ROBJ_MEMBER_ID memId,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& trailer, X::Value& retValue)
		{
			if (!CheckConnectReadyStatus())
			{
				return false;
			}
			return CallHandler::Call(rt, pContext, parent_id, id, memId, params, kwParams, trailer, retValue);
		}

		//use process as signal to host exit
		//this way will not work for case host is in Admin or service mode
		//so we need to use semaphore, in fact, in windows, we use event
		void RemotingProxy::WaitToHostExit()
		{
			//for wait to host exit
			SEMAPHORE_HANDLE semaphore = nullptr;
			int retCode = 0;
			while (retCode >=0)
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

				retCode = WAIT_FOR_SEMAPHORE(semaphore, 60);
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
		void RemotingProxy::SessionRun()
		{
			AddRef();
			while (mRun)
			{
				//std::cout << "Client:Try to connect" << std::endl;
				bool bOK = Connect();
				if (!bOK)
				{
					std::cout << "Client:Tried with fail, will try again" << std::endl;
					MS_SLEEP(200);
					continue;
				}
				//std::cout << "Client:Connected" << std::endl;
				m_ConnectLock.Lock();
				m_bConnected = true;
				m_ConnectLock.Unlock();
				m_pConnectWait->Release();
				//std::cout << "Client:Wait for host exit" << std::endl;
				WaitToHostExit();
				//std::cout << "Client:Host exited" << std::endl;
				CallHandler::Quit();
				mCallCounter.WaitForZero();
				CallHandler::Close();
				if (m_ExitOnHostExit)
				{
					m_ConnectLock.Lock();
					mRun = false;
					m_Exited = true;
					m_bConnected = false;
					m_ConnectLock.Unlock();
					//std::cout << "Client Session Closing" << std::endl;
					Manager::I().RemoveProxy(LRPC_NAME, mRootObjectName, this);
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
				//Restart again for Read Thread in Base class
				CallHandler::ReStart();
			}
			//std::cout << "Client Session Exit" << std::endl;
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
			//std::cout << "begin ShakeHands" << std::endl;
			unsigned long pid_client = GetPID();
			Call_Context context;
			auto& stream = BeginCall((unsigned int)RPC_CALL_TYPE::ShakeHands, context);
			stream << pid_client;
			long long returnCode = 0;
			auto& stream2 = CommitCall(context, returnCode);
			unsigned long pid_srv = 0;
			unsigned long long sid = 0;
			if (returnCode > 0)
			{
				stream2 >> pid_srv;
				stream2 >> sid;
			}
			FinishCall();
			mHostProcessId = pid_srv;
			mSessionId = sid;
			//std::cout << "end ShakeHands" << std::endl;
		}
	}//namespace IPC
}//namespace X