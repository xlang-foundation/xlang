/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MSGTHREAD_H
#define MSGTHREAD_H

#include "singleton.h"
#include "Locker.h"
#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include "gthread.h"
#include "port.h"
#include "service_def.h"

namespace X
{
    namespace IPC
    {
        class MsgService :
            public GThread,
            public Singleton<MsgService>
        {
        public:
            MsgService();
            ~MsgService();

            void Stop();
            void run();
            void SetPort(long port) { mPort = port; }

            // Status checking
            bool IsRunning() const { return mRun.load(); }
            bool IsInitialized() const { return mInitialized.load(); }

        private:
            // Configuration
            static constexpr int MAX_RETRY_COUNT = 10;
            static constexpr int RETRY_DELAY_MS = 100;
            static constexpr int POLL_INTERVAL_MS = 1;
            static constexpr int ERROR_SLEEP_MS = 1000;

            // State management
            std::atomic<bool> mRun{ true };
            std::atomic<bool> mInitialized{ false };
            std::atomic<long> mPort{ 0 };

            // Platform-specific handles
            SEMAPHORE_HANDLE mSemaphore_For_Process = nullptr;

#if (WIN32)
            HANDLE mMailslotHandle = INVALID_HANDLE_VALUE;
            PSECURITY_DESCRIPTOR mSecurityDescriptor = nullptr;
#else
            int mMsgId = 0;
            Locker mMsgLock;
#endif

            // Private methods
            bool Initialize();
            void Cleanup();
            bool MakeProcessLevelSemaphore();
            void ProcessMessages();

#if (WIN32)
            bool CreateMailslotWithRetry();
            bool ReadMailslotMessages(std::vector<pas_mesg_buffer>& msgs);
            void CleanupMailslot();
            PSECURITY_DESCRIPTOR CreateSecurityDescriptor();
#else
            bool CreateMessageQueueWithRetry();
            bool ReadMessageQueueMessage(pas_mesg_buffer& message);
            void CleanupMessageQueue();
#endif

            // Helper methods
#if (WIN32)
            std::string GetMessageKey() const;
#endif
            void HandleCreateSharedMemMessage(const pas_mesg_buffer& msg);
            void LogError(const std::string& operation, int errorCode = 0);
            void LogSuccess(const std::string& operation);
        };
    } // namespace IPC
} // namespace X

#endif // MSGTHREAD_H