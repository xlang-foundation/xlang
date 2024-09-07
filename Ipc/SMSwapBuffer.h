#pragma once

#include "Locker.h"
#include "service_def.h"
#include "port.h"
#if (WIN32)
#include <Windows.h>
#include <sddl.h>
#endif

namespace X
{
	namespace IPC 
	{

		typedef void* PasWaitHandle;

#if (WIN32)
#define RESETEVENT(evt)  ResetEvent(evt)
#define SETEVENT(evt)    SetEvent(evt)
#else
#define RESETEVENT(evt)  // Not needed for semaphores
#define SETEVENT(evt)    sem_post(evt);
#endif

#if (WIN32)
		bool CheckIfAdmin()
		{
			BOOL isAdmin = FALSE;
			PSID adminGroup = NULL;

			// Create a SID for the administrators group
			SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
			if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
				return false;
			}

			// Check if the current token contains the admin SID
			if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
				isAdmin = FALSE;
			}

			FreeSid(adminGroup);
			return isAdmin;
		}
#else
		//in linux, we don't need to add global as prefix for IPC objects
		//so we just return false
		bool CheckIfAdmin()
		{
			return false;
		}
#endif
		class SMSwapBuffer
		{
		public:
			SMSwapBuffer() : mClosed(false), mShmID(0), mWriteEvent(nullptr),
				mReadEvent(nullptr), mShmPtr(0), m_BufferSize(0),
				mSemaphore_For_Process(nullptr) {}
			~SMSwapBuffer() { Close(); }

			bool HostCreate(unsigned long long key, int bufSize)
			{
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
				bool IsAdmin = CheckIfAdmin();
				auto pid = GetPID();

				// Create the semaphore for process-level synchronization
				std::string semaphoreName =
					IsAdmin ? "Global\\XlangServerSemaphore_" : "XlangServerSemaphore_";
				semaphoreName += std::to_string(pid);

#if (WIN32)
				mSemaphore_For_Process = CreateSemaphore(&sa, 1, 1, semaphoreName.c_str());
				if (!mSemaphore_For_Process)
				{
					std::cout << "CreateSemaphore failed with error: " << GetLastError() << std::endl;
					return false;
				}
#elif __ANDROID__
				// Add Android/Linux semaphore logic if necessary
#else
				mSemaphore_For_Process = sem_open(semaphoreName.c_str(), O_CREAT | O_EXCL, 0666, 1);
#endif

				// Create the write and read events with different names for server and client
				const int Key_Len = 100;
				char szWriteEvent[Key_Len], szReadEvent[Key_Len];
				SPRINTF(szWriteEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Write_%llu" : "Galaxy_SM_Write_%llu", key);
				SPRINTF(szReadEvent, Key_Len, IsAdmin ? "Global\\Galaxy_SM_Read_%llu" : "Galaxy_SM_Read_%llu", key);

#if (WIN32)
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

#elif __ANDROID__
				// Add Android/Linux implementation if necessary
#else
				mShmID = shmget(key, bufSize, IPC_CREAT | 0666);
				mShmPtr = (char*)shmat(mShmID, 0, 0);
				mWriteEvent = sem_open(szWriteEvent, O_CREAT | O_EXCL, 0666, 0);
				mReadEvent = sem_open(szReadEvent, O_CREAT | O_EXCL, 0666, 0);
#endif

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
				usGlobal = false;
				const int Key_Len = 100;
				char szKey_s[Key_Len];
				SPRINTF(szKey_s, Key_Len, "Global\\Galaxy_SM_Notify_Server_%llu", shKey);
				char szKey_c[Key_Len];
				SPRINTF(szKey_c, Key_Len, "Global\\Galaxy_SM_Notify_Client_%llu", shKey);

#if (WIN32)
				//Non-Global
				char szKey_s_l[Key_Len];
				SPRINTF(szKey_s_l, Key_Len, "Galaxy_SM_Notify_Server_%llu", shKey);
				char szKey_c_l[Key_Len];
				SPRINTF(szKey_c_l, Key_Len, "Galaxy_SM_Notify_Client_%llu", shKey);

				char mappingName[MAX_PATH];
				sprintf_s(mappingName, "Global\\Galaxy_FileMappingObject_%llu", shKey);

				char mappingName_l[MAX_PATH];
				sprintf_s(mappingName_l, "Galaxy_FileMappingObject_%llu", shKey);

				const int loopNum = 1000;
				int loopNo = 0;
				bool bSrvReady = false;
				while (mWriteEvent == nullptr && loopNo < loopNum)
				{
					mWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_s);
					if (mWriteEvent == nullptr)
					{
						mWriteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szKey_s_l);
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
						usGlobal ? szKey_c : szKey_c_l);
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
#elif __ANDROID__

#else
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
				loopNo = 0;
				bSrvReady = false;
				while (mWriteEvent == nullptr && loopNo < loopNum)
				{
					mWriteEvent = sem_open(szKey_s, O_EXCL, permission, 0);
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
					mReadEvent = sem_open(szKey_c, O_EXCL, permission, 0);
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
#endif
				if (mShmPtr == NULL)
				{
					std::cout << "ClientConnect:failed" << std::endl;
					return false;
				}
				//std::cout  << "ClientConnect:OK" << std::endl;
				mClosed = false;
				m_BufferSize = bufSize;
				return true;
			}

			FORCE_INLINE void BeginWrite()
			{
				// Wait for permission to write
				Wait(mWriteEvent, INFINITE);  // The event should be set to allow writing
				RESETEVENT(mWriteEvent);      // Reset the write event so the next write waits
			}

			FORCE_INLINE void EndWrite()
			{
				// Signal that writing is done, the reader can now read
				SETEVENT(mReadEvent);  // Signal the read event for the reader
			}

			FORCE_INLINE bool BeginRead(int timeoutMS = -1)
			{
				// Wait for permission to read
				bool result = Wait(mReadEvent, timeoutMS);
				if (result)
				{
					RESETEVENT(mReadEvent);  // Reset the read event so the next read waits
				}
				return result;
			}

			FORCE_INLINE void EndRead()
			{
				// Signal that reading is done, the writer can now write again
				SETEVENT(mWriteEvent);  // Signal the write event for the writer
			}

			FORCE_INLINE PayloadFrameHead& GetHead()
			{
				return *(PayloadFrameHead*)mShmPtr;
			}

			FORCE_INLINE char* GetBuffer()
			{
				return mShmPtr + sizeof(PayloadFrameHead);
			}

			FORCE_INLINE int BufferSize()
			{
				return m_BufferSize - sizeof(PayloadFrameHead);
			}

			FORCE_INLINE void ReleaseEvents()
			{
				SETEVENT(mWriteEvent);
				SETEVENT(mReadEvent);
			}

			void Close()
			{
				mClosed = true;
				if (mWriteEvent)
				{
#if (WIN32)
					CloseHandle(mWriteEvent);
#else
					sem_destroy(mWriteEvent);
#endif
					mWriteEvent = nullptr;
				}
				if (mReadEvent)
				{
#if (WIN32)
					CloseHandle(mReadEvent);
#else
					sem_destroy(mReadEvent);
#endif
					mReadEvent = nullptr;
				}
				if (mSemaphore_For_Process)
				{
#if (WIN32)
					CloseHandle(mSemaphore_For_Process);
#else
					sem_close(mSemaphore_For_Process);
					sem_unlink("semaphoreName");
#endif
					mSemaphore_For_Process = nullptr;
				}
				if (mShmPtr)
				{
#if (WIN32)
					UnmapViewOfFile(mShmPtr);
#elif __ANDROID__

#else
					shmdt(mShmPtr);
#endif
					mShmPtr = nullptr;
				}
				if (mShmID)
				{
#if (WIN32)
					CloseHandle(mShmID);
#else
					shmctl(mShmID, IPC_RMID, 0);
#endif
					mShmID = 0;
				}
			}

		private:
			bool Wait(PasWaitHandle h, int timeoutMS)
			{
#if (WIN32)
				return (WAIT_OBJECT_0 == ::WaitForSingleObject(h, timeoutMS));
#else
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
#endif
			}
private:
			bool mClosed;
#if (WIN32)
			HANDLE mShmID;
#else
			int mShmID;
#endif
			EVENT_HANDLE mWriteEvent;  // Write event for synchronization
			EVENT_HANDLE mReadEvent;   // Read event for synchronization
			SEMAPHORE_HANDLE mSemaphore_For_Process;  // Semaphore for additional process-level synchronization
			Locker mSharedMemLock;
			char* mShmPtr;
			int m_BufferSize;
		};

	}  // namespace IPC
}  // namespace X
