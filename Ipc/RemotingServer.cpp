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

#include "RemotingServer.h"
#include "utility.h"
#include "service_def.h"
#include "SMSwapBuffer.h"
#include "RemotingServerMgr.h"
#include "IpcBase.h"
#include "port.h"
#include <iostream>

#if (!WIN32)
#include <signal.h>
#endif

namespace X
{
	namespace IPC
	{
		void StubWatch::run()
		{
			while (mRun)
			{
				WaitToHostExit();
				mParent->OnClientExit();
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
				sleep(10);
				res = kill((pid_t)mPidToWatch, 0);
			}
#endif
			mPidToWatch = 0;
		}
		void RemotingServer::OnClientExit()
		{
			std::cout << "IPC Client Exited,m_clientPid=" << m_clientPid << std::endl;
			RemotingManager::I().CloseServer(this);
			std::cout << "IPC Stub Closed" << std::endl;
		}

		RemotingServer::RemotingServer()
		{
			mMinReqId = 20000;//should not be inside client's range
			mMaxReqId = 30000;
			mNextRequestId = mMinReqId;

			m_pStubWatch = new StubWatch();
			m_pStubWatch->SetParent(this);

			mWBuffer = new SMSwapBuffer();
			mRBuffer = new SMSwapBuffer();
		}
		RemotingServer::~RemotingServer()
		{
			Close();
			std::cout << "~RemotingServer(),m_clientPid=" << m_clientPid << std::endl;
			delete mWBuffer;
			delete mRBuffer;

			delete m_pStubWatch;
		}
		bool RemotingServer::Create(unsigned long long shmKey)
		{
			bool IsAdmin = Helper::CheckIfAdmin();
			mKey = shmKey;
			bool bOK = mWBuffer->HostCreate(shmKey, SM_BUF_SIZE, IsAdmin);
			if (bOK)
			{
				bOK = mRBuffer->HostCreate(shmKey + 1, SM_BUF_SIZE, IsAdmin);
			}
			StartReadThread();
			return bOK;
		}

		void RemotingServer::ShakeHandsCall(void* pCallContext, SwapBufferStream& stream)
		{
			unsigned long clientPid = 0;
			stream >> clientPid;
			std::cout << "IPC,RemotingServer::ShakeHandsCall,clientPid=" << clientPid << std::endl;
			EndReceiveCall(stream);
			m_clientPid = clientPid;
			m_pStubWatch->SetPid(clientPid);
			std::cout << "IPC,WatchClientProcess,pid =" << clientPid << std::endl;
			m_pStubWatch->Start();
			bool bOK = true;
			auto& wStream = BeginWriteReturn(pCallContext,bOK);
			if (bOK)
			{
				unsigned long pid = GetPID();
				wStream << pid;
				wStream << m_sessionId;
			}
			EndWriteReturn(pCallContext, bOK);
		}
		void RemotingServer::SetRootObjectName(const char* name)
		{
			//Don't need to implement in Server side
			//we don't have chance to call import from client side
		}
		ROBJ_ID RemotingServer::QueryRootObject(std::string& name)
		{
			//Don't need to implement in Server side
			//we don't have chance to call import from client side
			return ROBJ_ID();
		}
		void RemotingServer::SetTimeout(int timeout)
		{
		}
	}//namespace IPC
}//namespace X