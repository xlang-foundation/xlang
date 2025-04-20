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

namespace X {
    namespace IPC {

        typedef void* PasWaitHandle;

#if (WIN32)
        typedef HANDLE EVENT_HANDLE;
#define RESETEVENT(evt)  ResetEvent(evt)
#define SETEVENT(evt)    SetEvent(evt)
#else
        typedef sem_t* EVENT_HANDLE;
#define RESETEVENT(evt)  /* not used */
#define SETEVENT(evt)    sem_post(evt)
#endif

        class SMSwapBuffer {
        public:
            SMSwapBuffer()
                : mClosed(false)
                , mShmID(0)
                , mWriteEvent(nullptr)
                , mReadEvent(nullptr)
                , mShmPtr(nullptr)
                , m_BufferSize(0)
            {
            }

            ~SMSwapBuffer() { Close(); }

            bool HostCreate(unsigned long long key, int bufSize, bool IsAdmin);
            bool SendMsg(long port, unsigned long long shKey);
            bool ClientConnect(bool& usGlobal,
                long port,
                unsigned long long shKey,
                int bufSize,
                int timeoutMS,
                bool needSendMsg = true);

            // --- Writer side ---
            FORCE_INLINE void BeginWrite() {
                if (mClosed) return;                          // no new writes once closed

                // wait for writer event
                if (!Wait(mWriteEvent, INFINITE)) return;

                // now grab lock just before touching shared memory
                mSharedMemLock.Lock();
                if (mClosed || mShmPtr == nullptr) {
                    mSharedMemLock.Unlock();
                    return;
                }

                RESETEVENT(mWriteEvent);                     // reset for next write
            }

            FORCE_INLINE void EndWrite() {
                SETEVENT(mReadEvent);                         // signal reader
                mSharedMemLock.Unlock();                      // release mapping
            }

            // --- Reader side ---
            FORCE_INLINE bool BeginRead(int timeoutMS = -1) {
                if (mClosed) return false;                    // no new reads once closed

                // wait without holding lock
                if (!Wait(mReadEvent, timeoutMS)) return false;

                // grab lock before accessing shared memory
                mSharedMemLock.Lock();
                if (mClosed || mShmPtr == nullptr) {
                    mSharedMemLock.Unlock();
                    return false;
                }
                m_beginRead = true;
                RESETEVENT(mReadEvent);                       // reset for next read
                return true;
            }

            FORCE_INLINE void EndRead() {
				m_beginRead = false;
                SETEVENT(mWriteEvent);                        // signal writer
                mSharedMemLock.Unlock();                      // release mapping
            }

            // --- Accessors ---
            FORCE_INLINE PayloadFrameHead& GetHead() {
                return *reinterpret_cast<PayloadFrameHead*>(mShmPtr);
            }

            FORCE_INLINE char* GetBuffer() {
                return mShmPtr + sizeof(PayloadFrameHead);
            }

            FORCE_INLINE int BufferSize() {
                return m_BufferSize - sizeof(PayloadFrameHead);
            }

            FORCE_INLINE void ReleaseEvents() {
                SETEVENT(mWriteEvent);
                SETEVENT(mReadEvent);
            }
			FORCE_INLINE bool IsBeginRead() {
				return m_beginRead;
			}
            // --- Cleanup ---
            void Close() {
                if (mClosed) return;
                mClosed = true;                               // stop new I/O

                // wait for any in-flight Begin*/End* pairs to finish
                mSharedMemLock.Lock();

                // close events
                if (mWriteEvent) {
#if (WIN32)
                    CloseHandle(mWriteEvent);
#else
                    sem_close(mWriteEvent);
                    if (!mWriteEventName.empty()) sem_unlink(mWriteEventName.c_str());
#endif
                    mWriteEvent = nullptr;
                }
                if (mReadEvent) {
#if (WIN32)
                    CloseHandle(mReadEvent);
#else
                    sem_close(mReadEvent);
                    if (!mReadEventName.empty()) sem_unlink(mReadEventName.c_str());
#endif
                    mReadEvent = nullptr;
                }

                // unmap shared memory
                if (mShmPtr) {
#if (WIN32)
                    UnmapViewOfFile(mShmPtr);
#else
                    munmap(mShmPtr, m_BufferSize);
#endif
                    mShmPtr = nullptr;
                }

                // close mapping handle
                if (mShmID) {
#if (WIN32)
                    CloseHandle(mShmID);
#else
                    close(mShmID);
                    if (!mShmName.empty()) shm_unlink(mShmName.c_str());
#endif
                    mShmID = 0;
                }

                mSharedMemLock.Unlock();
            }

        private:
            bool Wait(PasWaitHandle h, int timeoutMS);

            bool          mClosed;
#if (WIN32)
            HANDLE        mShmID;
#else
            int           mShmID;
#endif
            EVENT_HANDLE  mWriteEvent;
            EVENT_HANDLE  mReadEvent;
            Locker        mSharedMemLock;
            char* mShmPtr;
            int           m_BufferSize;
			bool m_beginRead = false;
#if !(WIN32)
            std::string   mShmName;
            std::string   mWriteEventName;
            std::string   mReadEventName;
#endif
        };

    } // namespace IPC
} // namespace X
