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

#if (WIN32)
#include "SMSwapBuffer.h"

namespace X {
	namespace IPC {
		bool SMSwapBuffer::Wait(PasWaitHandle h, int timeoutMS)
		{
			return (WAIT_OBJECT_0 == ::WaitForSingleObject(h, timeoutMS));
		}
		bool SMSwapBuffer::HostCreate(unsigned long long key, int bufSize, bool IsAdmin)
		{
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

			// Create the write and read events with different names for server and client
			const int Key_Len = 100;
			char szWriteEvent[Key_Len], szReadEvent[Key_Len];
			SPRINTF(szWriteEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Write_%llu" : "Galaxy_SM_Write_%llu", key);
			SPRINTF(szReadEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Read_%llu" : "Galaxy_SM_Read_%llu", key);

			// Create Write Event
			mWriteEvent = CreateEvent(&sa, FALSE, TRUE, szWriteEvent);  // Initially set to TRUE for the first write
			if (!mWriteEvent)
			{
				DWORD error = GetLastError();
				std::cout << "CreateEvent (Write) failed with error: " << error << std::endl;
				return false;
			}
			// Create Read Event
			mReadEvent = CreateEvent(&sa, FALSE, FALSE, szReadEvent);  // Initially set to FALSE (read happens after write)
			if (!mReadEvent)
			{
				DWORD error = GetLastError();
				std::cout << "CreateEvent (Read) failed with error: " << error << std::endl;
				return false;
			}

			// Create the shared memory object
			char mappingName[MAX_PATH];
			SPRINTF(mappingName, MAX_PATH, IsAdmin ? "Global\\Galaxy_FileMappingObject_%llu" : "Galaxy_FileMappingObject_%llu", key);

			mShmID = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, bufSize, mappingName);
			if (!mShmID)
			{
				DWORD error = GetLastError();
				std::cout << "CreateFileMapping failed with error: " << error << std::endl;
				return false;
			}
			mShmPtr = (char*)MapViewOfFile(mShmID, FILE_MAP_ALL_ACCESS, 0, 0, bufSize);
			if (!mShmPtr)
			{
				DWORD error = GetLastError();
				std::cout << "MapViewOfFile failed with error: " << error << std::endl;
				return false;
			}

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
			//for windows, we like to try Global first to check
			//if server side runs as admin mode
			//so we need to two set of keys
			const int Key_Len = 100;
			//Global keys
			char szKey_w[Key_Len];
			SPRINTF(szKey_w, Key_Len, "Global\\Galaxy_SM_Write_%llu", shKey);
			char szKey_r[Key_Len];
			SPRINTF(szKey_r, Key_Len, "Global\\Galaxy_SM_Read_%llu", shKey);
			//Non-Global keys
			char szKey_w_l[Key_Len];
			SPRINTF(szKey_w_l, Key_Len, "Galaxy_SM_Write_%llu", shKey);
			char szKey_r_l[Key_Len];
			SPRINTF(szKey_r_l, Key_Len, "Galaxy_SM_Read_%llu", shKey);

			char mappingName[MAX_PATH];
			sprintf_s(mappingName, "Global\\Galaxy_FileMappingObject_%llu", shKey);

			char mappingName_l[MAX_PATH];
			sprintf_s(mappingName_l, "Galaxy_FileMappingObject_%llu", shKey);

			const int loopNum = 1000;
			int loopNo = 0;
			bool bSrvReady = false;
			//set to false to try global first
			usGlobal = false;
			while (mWriteEvent == nullptr && loopNo < loopNum)
			{
				mWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_w);
				if (mWriteEvent == nullptr)
				{
					mWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_w_l);
				}
				else
				{
					usGlobal = true;
				}
				if (mWriteEvent != nullptr)
				{
					bSrvReady = true;
					break;
				}
				MS_SLEEP(100);
				loopNo++;
			}
			if (!bSrvReady)
			{
				return false;
			}
			loopNo = 0;
			bSrvReady = false;
			while (loopNo < loopNum)
			{
				mShmID = CreateFileMapping(
					INVALID_HANDLE_VALUE,
					NULL,
					PAGE_READWRITE,
					0,
					bufSize,
					usGlobal ? mappingName : mappingName_l);
				if (mShmID != NULL)
				{
					mShmPtr = (char*)MapViewOfFile(mShmID, FILE_MAP_ALL_ACCESS,
						0, 0, bufSize);
					bSrvReady = true;
					break;
				}
				MS_SLEEP(100);
				loopNo++;
			}

			loopNo = 0;
			bSrvReady = false;
			while (mReadEvent == nullptr && loopNo < loopNum)
			{
				mReadEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE,
					usGlobal ? szKey_r : szKey_r_l);
				if (mReadEvent != nullptr)
				{
					bSrvReady = true;
					break;
				}
				MS_SLEEP(100);
				loopNo++;
			}
			if (!bSrvReady)
			{
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
	}  // namespace IPC
}  // namespace X


#endif // _WIN32
