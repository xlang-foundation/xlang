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

#pragma once

#include "Locker.h"
#include "service_def.h"
#include "port.h"
#include "utility.h"
#include <iostream>
#include <string>

#if (WIN32)
#include <Windows.h>
#else
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/mman.h>

#define INFINITE   -1
#endif

#if defined(__APPLE__)
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout);
#endif
namespace X
{
	namespace IPC
	{

		typedef void* PasWaitHandle;

#if (WIN32)
		typedef HANDLE EVENT_HANDLE;
#define RESETEVENT(evt)  ResetEvent(evt)
#define SETEVENT(evt)    SetEvent(evt)
#else
		typedef sem_t* EVENT_HANDLE;
#define RESETEVENT(evt)  // Not needed for semaphores
#define SETEVENT(evt)    sem_post(evt);
#endif

		class SMSwapBuffer
		{
		public:
			SMSwapBuffer() : mClosed(false), mShmID(0), mWriteEvent(nullptr),
				mReadEvent(nullptr), mShmPtr(0), m_BufferSize(0) {}
			~SMSwapBuffer() { Close(); }

			bool HostCreate(unsigned long long key, int bufSize, bool IsAdmin);
			bool SendMsg(long port, unsigned long long shKey);
			bool ClientConnect(bool& usGlobal, long port, unsigned long long shKey,
				int bufSize, int timeoutMS, bool needSendMsg = true);

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
				if (mClosed)
				{
					// Prevent multiple closures
					return;
				}
				mClosed = true;

				// Close and unlink the write semaphore
				if (mWriteEvent)
				{
#if (WIN32)
					CloseHandle(mWriteEvent);
#else
					sem_close(mWriteEvent);
					if (!mWriteEventName.empty())
					{
						sem_unlink(mWriteEventName.c_str());
					}
#endif
					mWriteEvent = nullptr;
				}

				// Close and unlink the read semaphore
				if (mReadEvent)
				{
#if (WIN32)
					CloseHandle(mReadEvent);
#else
					sem_close(mReadEvent);
					if (!mReadEventName.empty())
					{
						sem_unlink(mReadEventName.c_str());
					}
#endif
					mReadEvent = nullptr;
				}

				// Unmap and close the shared memory
				if (mShmPtr)
				{
#if (WIN32)
					UnmapViewOfFile(mShmPtr);
#elif __ANDROID__
					// Add Android-specific code if necessary
#else
					munmap(mShmPtr, m_BufferSize);
#endif
					mShmPtr = nullptr;
				}

				if (mShmID)
				{
#if (WIN32)
					CloseHandle(mShmID);
#else
					close(mShmID);
					if (!mShmName.empty())
					{
						shm_unlink(mShmName.c_str());
					}
#endif
					mShmID = 0;
				}
			}

		private:
			bool Wait(PasWaitHandle h, int timeoutMS);
		private:
			bool mClosed;
#if (WIN32)
			HANDLE mShmID;
#else
			int mShmID;
#endif
			EVENT_HANDLE mWriteEvent;  // Write event for synchronization
			EVENT_HANDLE mReadEvent;   // Read event for synchronization
			Locker mSharedMemLock;
			char* mShmPtr;
			int m_BufferSize;
			//for linux, we need to call sem_unlink to remove them
#if !(WIN32)
			std::string mShmName;
			std::string mWriteEventName;
			std::string mReadEventName;
#endif
		};

	}  // namespace IPC
}  // namespace X
