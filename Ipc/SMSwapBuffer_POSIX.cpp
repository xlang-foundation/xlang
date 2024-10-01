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

#if !(WIN32)
#include "SMSwapBuffer.h"

namespace X {
	namespace IPC {
		bool SMSwapBuffer::HostCreate(unsigned long long key, int bufSize, bool IsAdmin)
		{
			// Create the write and read events with different names for server and client
			const int Key_Len = 100;
			char szWriteEvent[Key_Len], szReadEvent[Key_Len];
			SPRINTF(szWriteEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Write_%llu" : "Galaxy_SM_Write_%llu", key);
			SPRINTF(szReadEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Read_%llu" : "Galaxy_SM_Read_%llu", key);
			mShmID = shmget(key, bufSize, IPC_CREAT | 0666);

			mShmPtr = (char*)shmat(mShmID, 0, 0);
			//by sem_open with value-1, Initially be signaled for the first write
			mWriteEvent = sem_open(szWriteEvent, O_CREAT | O_EXCL, 0666, 1);
			//for read, only when write op finished, then signal it, so set value to 0
			mReadEvent = sem_open(szReadEvent, O_CREAT | O_EXCL, 0666, 0);
			m_BufferSize = bufSize;
			return true;
		}
		bool SMSwapBuffer::ClientConnect(bool& usGlobal, long port, unsigned long long shKey,
			int bufSize, int timeoutMS, bool needSendMsg)
		{
			if (needSendMsg)
			{
				const int loopNum = 1000;
				int loopNo = 0;
				bool bSrvReady = false;
				while (loopNo < loopNum)
				{
					if (SendMsg(port, shKey))
					{
						break;
					}
					MS_SLEEP(100);
					loopNo++;
				}
			}

			const int Key_Len = 100;
			char szKey_w[Key_Len];
			SPRINTF(szKey_w, Key_Len, "Galaxy_SM_Write_%llu", shKey);
			char szKey_r[Key_Len];
			SPRINTF(szKey_r, Key_Len, "Galaxy_SM_Read_%llu", shKey);

			printf("ClientConnect:shmget for read buffer\n");
			const int loopNum = 1000;
			int loopNo = 0;
			bool bSrvReady = false;
			int permission = 0666;
			while (loopNo < loopNum)
			{
				mShmID = shmget(shKey, (size_t)bufSize, permission);
				if (mShmID != -1)
				{
					mShmPtr = (char*)shmat(mShmID, NULL, 0);
					if (mShmPtr != nullptr)
					{
						bSrvReady = true;
						break;
					}
					else
					{//just in case
						shmctl(mShmID, IPC_RMID, NULL);
					}
				}
				MS_SLEEP(100);
				loopNo++;
				printf("shmget:%llu,Loop:%d\n", shKey, loopNo);
			}
			if (!bSrvReady)
			{
				printf("shmget:failed\n");
				return false;
			}
			printf("shmget:OK, then sem_open for write buffer\n");
			loopNo = 0;
			bSrvReady = false;
			while (mWriteEvent == nullptr && loopNo < loopNum)
			{
				mWriteEvent = sem_open(szKey_w, O_EXCL, permission, 0);
				if (mWriteEvent != nullptr)
				{
					bSrvReady = true;
					break;
				}
				MS_SLEEP(100);
				loopNo++;
				printf("sem_open:%llu,Loop:%d\n", shKey, loopNo);
			}
			loopNo = 0;
			bSrvReady = false;
			while (mReadEvent == nullptr && loopNo < loopNum)
			{
				mReadEvent = sem_open(szKey_r, O_EXCL, permission, 0);
				if (mReadEvent != nullptr)
				{
					bSrvReady = true;
					break;
				}
				MS_SLEEP(100);
				loopNo++;
				printf("sem_open:0x%llu,Loop:%d\n", shKey, loopNo);
			}
			if (!bSrvReady)
			{
				printf("sem_open:failed\n");
				return false;
			}
			if (mShmPtr == nullptr)
			{
				printf("ClientConnect:failed\n");
				return false;
			}
			//printf("ClientConnect:OK\n");
			mClosed = false;
			m_BufferSize = bufSize;
			return true;
		}
		bool SMSwapBuffer::Wait(PasWaitHandle h, int timeoutMS)
		{
			struct timeval now;
			struct timespec ts;
			gettimeofday(&now, NULL);
			if (timeoutMS == -1)
			{
				now.tv_sec += 3600 * 24;
			}
			else
			{
				now.tv_usec += timeoutMS * 1000;
				if (now.tv_usec >= 1000000)
				{
					now.tv_sec += now.tv_usec / 1000000;
					now.tv_usec %= 1000000;
				}
			}

			ts.tv_sec = now.tv_sec;
			ts.tv_nsec = now.tv_usec * 1000;

			int ret = sem_timedwait((sem_t*)h, &ts);
			if (ret == -1)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		bool SMSwapBuffer::SendMsg(long port, unsigned long long shKey)
		{
			pas_mesg_buffer message;
			// msgget creates a message queue
			// and returns identifier
			message.mesg_type = (long)PAS_MSG_TYPE::CreateSharedMem;
			message.shmKey = shKey;

			// ftok to generate unique key
			key_t msgkey = port;
			printf("msgsnd with Key:0x%x\n", msgkey);
			int msgid = msgget(msgkey, 0666);
			//msgsnd to send message
			msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0);
			return true;
		}
	}
}
#endif