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

#include "MsgService.h"
#include "service_def.h"
#include "utility.h"
#include "manager.h"

#if (WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#include <vector>
#include <iostream>

#include "RemotingServerMgr.h"


namespace X
{
	namespace IPC
	{
#if !(WIN32)
		void CleanupXlangResources();
#endif
#if (WIN32)
		BOOL ReadSlot(HANDLE hSlot, std::vector<pas_mesg_buffer>& msgs)
		{
			DWORD cbMessage, cMessage, cbRead;
			BOOL fResult;
			HANDLE hEvent;
			OVERLAPPED ov;

			cbMessage = cMessage = cbRead = 0;

			hEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("EvtSlot"));
			if (NULL == hEvent)
			{
				return FALSE;
			}
			ov.Offset = 0;
			ov.OffsetHigh = 0;
			ov.hEvent = hEvent;

			fResult = GetMailslotInfo(hSlot, // mailslot handle 
				(LPDWORD)NULL,               // no maximum message size 
				&cbMessage,                   // size of next message 
				&cMessage,                    // number of messages 
				(LPDWORD)NULL);              // no read time-out 

			if (!fResult)
			{
				CloseHandle(hEvent);
				return FALSE;
			}

			if (cbMessage == MAILSLOT_NO_MESSAGE)
			{
				CloseHandle(hEvent);
				return TRUE;
			}


			while (cMessage != 0)  // retrieve all messages
			{
				char* buffer = new char[cbMessage];
				fResult = ReadFile(hSlot,
					buffer,
					cbMessage,
					&cbRead,
					&ov);

				if (!fResult)
				{
					CloseHandle(hEvent);
					delete[] buffer;
					return FALSE;
				}
				if (cbMessage >= sizeof(pas_mesg_buffer))
				{
					pas_mesg_buffer msg = *(pas_mesg_buffer*)buffer;
					msgs.push_back(msg);
				}
				delete[] buffer;
				fResult = GetMailslotInfo(hSlot,  // mailslot handle 
					(LPDWORD)NULL,               // no maximum message size 
					&cbMessage,                   // size of next message 
					&cMessage,                    // number of messages 
					(LPDWORD)NULL);              // no read time-out 

				if (!fResult)
				{
					return FALSE;
				}
			}
			CloseHandle(hEvent);
			return TRUE;
		}
#endif
		MsgService::MsgService()
		{
		}
		void MsgService::Stop()
		{
			mRun = false;
#if !(WIN32)
			RemoveMsgId();
#endif
			GThread::Stop();
		}

		void MsgService::run()
		{
#if !(WIN32)
			CleanupXlangResources();
#endif
			MakeProcessLevelSemaphore();
#if (WIN32)
			std::string msgKey(PAS_MSG_KEY);
			if (mPort != 0)
			{
				msgKey += tostring(mPort);
			}

			SECURITY_DESCRIPTOR sd;
			InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
			SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);  // Grant access to everyone
			SECURITY_ATTRIBUTES sa;
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = &sd;
			sa.bInheritHandle = FALSE;
			HANDLE hSlot = INVALID_HANDLE_VALUE;
			while (hSlot == INVALID_HANDLE_VALUE && mRun)
			{
				hSlot = CreateMailslot(
					msgKey.c_str(),
					0,
					MAILSLOT_WAIT_FOREVER,
					&sa);
				if (hSlot == INVALID_HANDLE_VALUE)
					Sleep(100);
			}
			while (mRun)
			{
				std::vector<pas_mesg_buffer> msgs;
				if (ReadSlot(hSlot, msgs) && msgs.size() > 0)
				{
					for (auto m : msgs)
					{
						if (m.mesg_type == (long)PAS_MSG_TYPE::CreateSharedMem)
						{
							std::cout << "MsgService,Get Message to Create Stub with key:"
								<< m.shmKey << std::endl;
							RemotingManager::I().CreateServer(m.shmKey);
						}
					}
				}
				else
				{
					Sleep(1);
				}
			}
			CloseHandle(hSlot);
#elif __ANDROID__

#else
			key_t key = (mPort == 0) ? PAS_MSG_KEY : mPort;
			int msgid;
			msgid = msgget(key, 0666 | IPC_CREAT);
			mMsgLock.Lock();
			mMsgId = msgid;
			mMsgLock.Unlock();
			pas_mesg_buffer message;
			printf("Start MsgLoop\n");
			while (mRun)
			{
				// msgrcv to receive message
				//block call, canceled by RemoveMsgId
				//Use sizeof(message) - sizeof(long) in msgrcv to 
				// exclude the mesg_type from the size calculation
				auto size = msgrcv(msgid, &message, sizeof(message) - sizeof(long), 0, 0);
				if (size > 0)
				{
					if (message.mesg_type ==
						(long)PAS_MSG_TYPE::CreateSharedMem)
					{
						std::cout << "MsgService,Get Message to Create Stub with key:"
							<< message.shmKey << std::endl;
						RemotingManager::I().CreateServer(message.shmKey);
					}
				}
				else if (size == 0) {
					// msgrcv received 0 size, possibly meaning no message or error
					std::cout << "Received 0 bytes, sleeping..." << std::endl;
					usleep(1000);  // Sleep for 1 millisecond
				}
				else {
					// Check for error conditions
					if (errno == EINTR) {
						std::cerr << "msgrcv interrupted, retrying..." << std::endl;
					}
					else {
						perror("msgrcv failed");
						mRun = false;  // Break the loop on a serious error
					}
				}
			}
			RemoveMsgId();
			std::cout << "Exit MsgLoop" << std::endl;
#endif
			if (mPort != 0)
			{
				Manager::I().RemoveLrpcPort(mPort);
			}
			Close();
		}

		bool MsgService::MakeProcessLevelSemaphore()
		{
			bool IsAdmin = Helper::CheckIfAdmin();
#if (WIN32)
			SECURITY_ATTRIBUTES sa;
			SECURITY_DESCRIPTOR sd;

			// Initialize the security descriptor for Global/Local admin usage
			if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
			{
				return false;
			}
			if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
			{
				return false;
			}
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);
			sa.lpSecurityDescriptor = &sd;
			sa.bInheritHandle = FALSE;
#endif

			auto pid = GetPID();
			std::string semaphoreName =
				IsAdmin ? "Global\\XlangServerSemaphore_" : "XlangServerSemaphore_";
			semaphoreName += std::to_string(pid);
			mSemaphore_For_Process = CREATE_SEMAPHORE(sa, semaphoreName.c_str());
			if (mSemaphore_For_Process == nullptr)
			{
				std::cout << "Create semaphore " << semaphoreName << " failed" << std::endl;
				return false;
			}
			else
			{
				std::cout << "Create semaphore " << semaphoreName << " OK" << std::endl;
			}
			return true;
		}

		void MsgService::Close()
		{
			if (mSemaphore_For_Process)
			{
				CLOSE_SEMAPHORE(mSemaphore_For_Process);
				mSemaphore_For_Process = nullptr;
			}
		}

		void MsgService::RemoveMsgId()
		{
#if (WIN32)

#elif __ANDROID__

#else
			mMsgLock.Lock();
			if (mMsgId != 0)
			{
				msgctl(mMsgId, IPC_RMID, NULL);
				mMsgId = 0;
			}
			mMsgLock.Unlock();
#endif
		}
	} // namespace IPC
} // namespace X