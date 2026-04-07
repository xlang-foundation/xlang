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
#include "RemotingServerMgr.h"
#include "log.h"

#if (WIN32)
#include <windows.h>
#include <sddl.h>
#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <cstring>
#endif

#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

namespace X
{
    namespace IPC
    {
#if !(WIN32)
        void CleanupXlangResources();
#endif

        MsgService::MsgService()
        {
            LOG << "MsgService constructor called" << LINE_END;
        }

        MsgService::~MsgService()
        {
            Stop();
            Cleanup();
        }

        void MsgService::Stop()
        {
            if (!mRun.exchange(false))
            {
                return; // Already stopped
            }

            LOG << "Stopping MsgService..." << LINE_END;

#if !(WIN32)
            CleanupMessageQueue();
#endif

            GThread::Stop();

            // Wait for thread to finish with timeout
            auto start = std::chrono::steady_clock::now();
            while (IsRunning() &&
                std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            LOG << "MsgService stopped" << LINE_END;
        }

        void MsgService::run()
        {
            LOG << "MsgService thread starting..." << LINE_END;

            try
            {
                if (!Initialize())
                {
                    LOG << LOG_RED << "Failed to initialize MsgService" << LOG_RESET << LINE_END;
                    return;
                }

                mInitialized = true;
                LOG << LOG_GREEN << "MsgService initialized successfully" << LOG_RESET << LINE_END;

                ProcessMessages();
            }
            catch (const std::exception& e)
            {
                LOG << LOG_RED << "Exception in MsgService::run: " << e.what() << LOG_RESET << LINE_END;
            }
            catch (...)
            {
                LOG << LOG_RED << "Unknown exception in MsgService::run" << LOG_RESET << LINE_END;
            }

            Cleanup();

            if (mPort != 0)
            {
                Manager::I().RemoveLrpcPort(mPort);
            }

            LOG << "MsgService thread exiting" << LINE_END;
        }

        bool MsgService::Initialize()
        {
#if !(WIN32)
            CleanupXlangResources();
#endif

            if (!MakeProcessLevelSemaphore())
            {
                LogError("Failed to create process semaphore");
                return false;
            }

#if (WIN32)
            return CreateMailslotWithRetry();
#else
            return CreateMessageQueueWithRetry();
#endif
        }

        void MsgService::Cleanup()
        {
            LOG << "Cleaning up MsgService resources..." << LINE_END;

#if (WIN32)
            CleanupMailslot();
#else
            CleanupMessageQueue();
#endif

            if (mSemaphore_For_Process)
            {
                CLOSE_SEMAPHORE(mSemaphore_For_Process);
                mSemaphore_For_Process = nullptr;
            }

            mInitialized = false;
        }

        void MsgService::ProcessMessages()
        {
            LOG << "Starting message processing loop" << LINE_END;

#if (WIN32)
            while (mRun.load())
            {
                try
                {
                    std::vector<pas_mesg_buffer> msgs;
                    if (ReadMailslotMessages(msgs))
                    {
                        for (const auto& msg : msgs)
                        {
                            HandleCreateSharedMemMessage(msg);
                        }
                    }

                    if (msgs.empty())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
                    }
                }
                catch (const std::exception& e)
                {
                    LogError("Exception in message processing", 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(ERROR_SLEEP_MS));
                }
            }
#else
            while (mRun.load())
            {
                try
                {
                    pas_mesg_buffer message;
                    if (ReadMessageQueueMessage(message))
                    {
                        HandleCreateSharedMemMessage(message);
                    }
                }
                catch (const std::exception& e)
                {
                    LogError("Exception in message processing", 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(ERROR_SLEEP_MS));
                }
            }
#endif

            LOG << "Message processing loop ended" << LINE_END;
        }

        std::string MsgService::GetMessageKey() const
        {
            std::string msgKey(PAS_MSG_KEY);
            if (mPort != 0)
            {
                msgKey += std::to_string(mPort);
            }
            return msgKey;
        }

        void MsgService::HandleCreateSharedMemMessage(const pas_mesg_buffer& msg)
        {
            if (msg.mesg_type == static_cast<long>(PAS_MSG_TYPE::CreateSharedMem))
            {
                LOG << "Received CreateSharedMem message with key: " << msg.shmKey << LINE_END;
                try
                {
                    RemotingManager::I().CreateServer(msg.shmKey);
                    LOG << LOG_GREEN << "Successfully created server for key: " << msg.shmKey << LOG_RESET << LINE_END;
                }
                catch (const std::exception& e)
                {
                    LOG << LOG_RED << "Failed to create server for key " << msg.shmKey
                        << ": " << e.what() << LOG_RESET << LINE_END;
                }
            }
        }

        bool MsgService::MakeProcessLevelSemaphore()
        {
            bool isAdmin = Helper::CheckIfAdmin();

#if (WIN32)
            SECURITY_ATTRIBUTES sa = {};
            SECURITY_DESCRIPTOR sd = {};

            if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
            {
                LogError("InitializeSecurityDescriptor", GetLastError());
                return false;
            }

            if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
            {
                LogError("SetSecurityDescriptorDacl", GetLastError());
                return false;
            }

            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = &sd;
            sa.bInheritHandle = FALSE;
#endif

            auto pid = GetPID();
            std::string semaphoreName = isAdmin ? "Global\\XlangServerSemaphore_" : "XlangServerSemaphore_";
            semaphoreName += std::to_string(pid);

            mSemaphore_For_Process = CREATE_SEMAPHORE(sa, semaphoreName.c_str());
            if (mSemaphore_For_Process == nullptr)
            {
                LogError("Create semaphore " + semaphoreName, GetLastError());
                return false;
            }

            LogSuccess("Create semaphore " + semaphoreName);
            return true;
        }

#if (WIN32)
        PSECURITY_DESCRIPTOR MsgService::CreateSecurityDescriptor()
        {
            LPCWSTR sddl = L"D:(D;;GRGW;;;SY)(A;;GRGW;;;IU)(A;;GRGW;;;NU)";
            PSECURITY_DESCRIPTOR pSD = nullptr;

            if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                sddl, SDDL_REVISION_1, &pSD, nullptr))
            {
                LogError("ConvertStringSecurityDescriptorToSecurityDescriptorW", GetLastError());
                return nullptr;
            }

            return pSD;
        }

        bool MsgService::CreateMailslotWithRetry()
        {
            std::string msgKey = GetMessageKey();

            mSecurityDescriptor = CreateSecurityDescriptor();
            if (!mSecurityDescriptor)
            {
                return false;
            }

            SECURITY_ATTRIBUTES sa = {};
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = mSecurityDescriptor;
            sa.bInheritHandle = FALSE;

            int retryCount = 0;
            bool errorLogged = false;

            while (mMailslotHandle == INVALID_HANDLE_VALUE && mRun.load() && retryCount < MAX_RETRY_COUNT)
            {
                mMailslotHandle = CreateMailslotA(
                    msgKey.c_str(),
                    0,
                    MAILSLOT_WAIT_FOREVER,
                    &sa);

                if (mMailslotHandle == INVALID_HANDLE_VALUE)
                {
                    DWORD error = GetLastError();
                    if (!errorLogged)
                    {
                        LogError("CreateMailslot for key: " + msgKey, error);
                        errorLogged = true;
                    }

                    if (error != ERROR_ALREADY_EXISTS)
                    {
                        retryCount++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                }
                else
                {
                    if (errorLogged)
                    {
                        LogSuccess("CreateMailslot for key: " + msgKey);
                    }
                    return true;
                }
            }

            return mMailslotHandle != INVALID_HANDLE_VALUE;
        }

        bool MsgService::ReadMailslotMessages(std::vector<pas_mesg_buffer>& msgs)
        {
            if (mMailslotHandle == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            DWORD cbMessage = 0, cMessage = 0, cbRead = 0;

            if (!GetMailslotInfo(mMailslotHandle, nullptr, &cbMessage, &cMessage, nullptr))
            {
                LogError("GetMailslotInfo", GetLastError());
                return false;
            }

            if (cbMessage == MAILSLOT_NO_MESSAGE)
            {
                return true; // No messages, but not an error
            }

            while (cMessage > 0 && mRun.load())
            {
                std::vector<char> buffer(cbMessage);

                if (!ReadFile(mMailslotHandle, buffer.data(), cbMessage, &cbRead, nullptr))
                {
                    LogError("ReadFile from mailslot", GetLastError());
                    return false;
                }

                if (cbRead >= sizeof(pas_mesg_buffer))
                {
                    pas_mesg_buffer msg;
                    std::memcpy(&msg, buffer.data(), sizeof(pas_mesg_buffer));
                    msgs.push_back(msg);
                }

                // Get info for next message
                if (!GetMailslotInfo(mMailslotHandle, nullptr, &cbMessage, &cMessage, nullptr))
                {
                    LogError("GetMailslotInfo (loop)", GetLastError());
                    return false;
                }
            }

            return true;
        }

        void MsgService::CleanupMailslot()
        {
            if (mMailslotHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(mMailslotHandle);
                mMailslotHandle = INVALID_HANDLE_VALUE;
            }

            if (mSecurityDescriptor)
            {
                LocalFree(mSecurityDescriptor);
                mSecurityDescriptor = nullptr;
            }
        }

#else // Linux implementation

        bool MsgService::CreateMessageQueueWithRetry()
        {
            key_t key = (mPort == 0) ? PAS_MSG_KEY : mPort;
            int retryCount = 0;
            bool errorLogged = false;

            while (mMsgId == 0 && mRun.load() && retryCount < MAX_RETRY_COUNT)
            {
                mMsgId = msgget(key, 0666 | IPC_CREAT);

                if (mMsgId == -1)
                {
                    if (!errorLogged)
                    {
                        LogError("msgget", errno);
                        errorLogged = true;
                    }

                    mMsgId = 0;
                    retryCount++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                }
                else
                {
                    if (errorLogged)
                    {
                        LogSuccess("msgget");
                    }

                    std::lock_guard<Locker> lock(mMsgLock);
                    // mMsgId is already set above
                    return true;
                }
            }

            return mMsgId != 0;
        }

        bool MsgService::ReadMessageQueueMessage(pas_mesg_buffer& message)
        {
            if (mMsgId == 0)
            {
                return false;
            }

            // Use sizeof(message) - sizeof(long) to exclude mesg_type from size calculation
            ssize_t size = msgrcv(mMsgId, &message, sizeof(message) - sizeof(long), 0, 0);

            if (size > 0)
            {
                return true;
            }
            else if (size == 0)
            {
                // No message received, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(POLL_INTERVAL_MS));
                return false;
            }
            else
            {
                // Error occurred
                if (errno == EINTR)
                {
                    LOG << "msgrcv interrupted, retrying..." << LINE_END;
                    return false;
                }
                else if (errno == EIDRM)
                {
                    LOG << "Message queue was removed" << LINE_END;
                    mRun = false;
                    return false;
                }
                else
                {
                    LogError("msgrcv", errno);
                    std::this_thread::sleep_for(std::chrono::milliseconds(ERROR_SLEEP_MS));
                    return false;
                }
            }
        }

        void MsgService::CleanupMessageQueue()
        {
            std::lock_guard<Locker> lock(mMsgLock);
            if (mMsgId != 0)
            {
                if (msgctl(mMsgId, IPC_RMID, nullptr) == -1)
                {
                    LogError("msgctl IPC_RMID", errno);
                }
                mMsgId = 0;
            }
        }

#endif // Platform-specific implementations

        void MsgService::LogError(const std::string& operation, int errorCode)
        {
            if (errorCode != 0)
            {
#if (WIN32)
                LOG << LOG_RED << "IPC " << operation << " failed with code: " << errorCode << LOG_RESET << LINE_END;
#else
                LOG << LOG_RED << "IPC " << operation << " failed: " << std::strerror(errorCode)
                    << " (code: " << errorCode << ")" << LOG_RESET << LINE_END;
#endif
            }
            else
            {
                LOG << LOG_RED << "IPC " << operation << " failed" << LOG_RESET << LINE_END;
            }
        }

        void MsgService::LogSuccess(const std::string& operation)
        {
            LOG << LOG_GREEN << "IPC " << operation << " succeeded" << LOG_RESET << LINE_END;
        }

    } // namespace IPC
} // namespace X