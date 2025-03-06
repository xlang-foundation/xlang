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
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sstream>
#include <limits.h>
#include <vector>
#include <sys/stat.h>

namespace X {
    namespace IPC {

        bool SMSwapBuffer::HostCreate(unsigned long long key, int bufSize, bool IsAdmin)
        {
            // Create the write and read events with different names for server and client
            const int Key_Len = 100;
            char szWriteEvent[Key_Len], szReadEvent[Key_Len];
            SPRINTF(szWriteEvent, Key_Len, "xlang.smw_%llu", key);
            SPRINTF(szReadEvent, Key_Len, "xlang.smr_%llu", key);

            char shmName[Key_Len];
            SPRINTF(shmName, Key_Len, "xlang.shm_%llu", key);

            // Store the names
            mWriteEventName = szWriteEvent;
            mReadEventName = szReadEvent;
            mShmName = shmName;

            // Create or open the shared memory object
            mShmID = shm_open(shmName, O_CREAT | O_RDWR, 0666);
            if (mShmID == -1) {
                perror("shm_open failed");
                return false;
            }

            // Set the size of the shared memory
            if (ftruncate(mShmID, bufSize) == -1) {
                perror("ftruncate failed");
                return false;
            }

            // Map the shared memory object into the process's address space
            mShmPtr = (char*)mmap(NULL, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, mShmID, 0);
            if (mShmPtr == MAP_FAILED) {
                perror("mmap failed");
                return false;
            }

            // By sem_open with value 1, initially signaled for the first write
            mWriteEvent = sem_open(szWriteEvent, O_CREAT | O_EXCL, 0666, 1);
            // For read, only when write operation finishes, then signal it, so set value to 0
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
            SPRINTF(szKey_w, Key_Len, "xlang.smw_%llu", shKey);
            char szKey_r[Key_Len];
            SPRINTF(szKey_r, Key_Len, "xlang.smr_%llu", shKey);

            printf("ClientConnect:shm_open for read buffer\n");
            const int loopNum = 1000;
            int loopNo = 0;
            bool bSrvReady = false;
            int permission = 0666;
            char shmName[Key_Len];
            SPRINTF(shmName, Key_Len, "xlang.shm_%llu", shKey);

            // Store the names
            mWriteEventName = szKey_w;
            mReadEventName = szKey_r;
            mShmName = shmName;

            // Loop to attempt connecting to shared memory
            while (loopNo < loopNum)
            {
                // Try to open the shared memory object
                mShmID = shm_open(shmName, O_RDWR, permission);
                if (mShmID != -1)
                {
                    // Map the shared memory object into the process's address space
                    mShmPtr = (char*)mmap(NULL, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED, mShmID, 0);
                    if (mShmPtr != MAP_FAILED)
                    {
                        bSrvReady = true;
                        break;
                    }
                    else
                    {
                        perror("mmap failed, retrying");
                        shm_unlink(shmName);  // Just in case
                    }
                }
                MS_SLEEP(100);
                loopNo++;
                printf("shm_open:%llu, Loop:%d\n", shKey, loopNo);
            }
            if (!bSrvReady)
            {
                printf("shm_open:failed\n");
                return false;
            }

            printf("shm_open:OK, then sem_open for write buffer\n");
            loopNo = 0;
            bSrvReady = false;

            // Loop to attempt connecting to the write semaphore
            while (mWriteEvent == nullptr && loopNo < loopNum)
            {
                mWriteEvent = sem_open(szKey_w, 0);
                if (mWriteEvent != SEM_FAILED)
                {
                    bSrvReady = true;
                    break;
                }
                MS_SLEEP(100);
                loopNo++;
                printf("sem_open (write):%llu, Loop:%d\n", shKey, loopNo);
            }
            if (!bSrvReady)
            {
                printf("sem_open (write):failed\n");
                return false;
            }

            loopNo = 0;
            bSrvReady = false;

            // Loop to attempt connecting to the read semaphore
            while (mReadEvent == nullptr && loopNo < loopNum)
            {
                mReadEvent = sem_open(szKey_r, 0);
                if (mReadEvent != SEM_FAILED)
                {
                    bSrvReady = true;
                    break;
                }
                MS_SLEEP(100);
                loopNo++;
                printf("sem_open (read):%llu, Loop:%d\n", shKey, loopNo);
            }
            if (!bSrvReady)
            {
                printf("sem_open (read):failed\n");
                return false;
            }

            if (mShmPtr == nullptr)
            {
                printf("ClientConnect:failed\n");
                return false;
            }

            // Connection successful, shared memory and semaphores are ready to use
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
            // Implementation of SendMsg remains unchanged
            pas_mesg_buffer message;
            message.mesg_type = (long)PAS_MSG_TYPE::CreateSharedMem;
            message.shmKey = shKey;

            key_t msgkey = port;
            printf("msgsnd with Key:0x%x\n", msgkey);
            int msgid = msgget(msgkey, 0666);
            msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0);
            return true;
        }
    //Help Functions to cleanup

// Function to check if any process has the file open
        bool IsFileInUse(const std::string& filepath)
        {
            // Get the device and inode of the target file
            struct stat fileStat;
            if (stat(filepath.c_str(), &fileStat) != 0)
            {
                perror(("stat failed for " + filepath).c_str());
                return false; // Can't access the file, assume not in use
            }

            // Iterate over all processes in /proc
            DIR* procDir = opendir("/proc");
            if (!procDir)
            {
                perror("opendir /proc failed");
                return false; // Conservatively assume the file is not in use
            }

            struct dirent* procEntry;
            while ((procEntry = readdir(procDir)) != nullptr)
            {
                // Skip entries that are not process directories
                pid_t pid = atoi(procEntry->d_name);
                if (pid <= 0)
                {
                    continue;
                }

                // Construct the path to the fd directory
                std::stringstream fdDirPath;
                fdDirPath << "/proc/" << pid << "/fd";

                DIR* fdDir = opendir(fdDirPath.str().c_str());
                if (!fdDir)
                {
                    // Cannot open fd directory; may not have permissions
                    continue;
                }

                struct dirent* fdEntry;
                while ((fdEntry = readdir(fdDir)) != nullptr)
                {
                    // Skip "." and ".."
                    if (strcmp(fdEntry->d_name, ".") == 0 || strcmp(fdEntry->d_name, "..") == 0)
                    {
                        continue;
                    }

                    // Construct the path to the symlink
                    std::stringstream fdPath;
                    fdPath << fdDirPath.str() << "/" << fdEntry->d_name;

                    // Get file info of the symlink target
                    struct stat fdStat;
                    if (stat(fdPath.str().c_str(), &fdStat) != 0)
                    {
                        continue;
                    }

                    // Compare device and inode numbers
                    if (fileStat.st_dev == fdStat.st_dev && fileStat.st_ino == fdStat.st_ino)
                    {
                        closedir(fdDir);
                        closedir(procDir);
                        return true; // File is in use
                    }
                }
                closedir(fdDir);
            }
            closedir(procDir);
            return false; // File is not in use
        }

        void CleanupXlangResources()
        {
            // Define prefixes for shared memory and semaphores
            const std::string shmPrefix = "xlang.shm_";
            const std::string semPrefixes[] = {
                "sem.XlangServerSemaphore_",
                "sem.xlang.smw_",
                "sem.xlang.smr_"
            };

            const char* dirpath = "/dev/shm";

            // Open the /dev/shm directory
            DIR* dir = opendir(dirpath);
            if (!dir)
            {
                perror("opendir failed");
                return;
            }

            struct dirent* entry;
            // Iterate over directory entries
            while ((entry = readdir(dir)) != nullptr)
            {
                std::string filename(entry->d_name);

                // Skip "." and ".." entries
                if (filename == "." || filename == "..")
                {
                    continue;
                }

                std::string fullPath = std::string(dirpath) + "/" + filename;

                // Check for shared memory objects
                if (filename.find(shmPrefix) == 0)
                {
                    // This is a shared memory object we might want to unlink
                    std::string shmName = "/" + filename; // Prepend "/"

                    // Check if the file is in use
                    if (IsFileInUse(fullPath))
                    {
                        std::cout << "Shared memory in use, skipping: " << shmName << std::endl;
                        continue;
                    }

                    // Attempt to unlink it
                    if (shm_unlink(shmName.c_str()) == 0)
                    {
                        std::cout << "Unlinked shared memory: " << shmName << std::endl;
                    }
                    else
                    {
                        // Ignore errors if the file doesn't exist
                        if (errno != ENOENT)
                        {
                            perror(("shm_unlink failed for " + shmName).c_str());
                        }
                    }
                }
                else if (filename.find("sem.") == 0)
                {
                    // Check for semaphores
                    for (const auto& semPrefix : semPrefixes)
                    {
                        if (filename.find(semPrefix) == 0)
                        {
                            // Remove "sem." prefix to get the name passed to sem_unlink
                            std::string semName = "/" + filename.substr(4); // Remove "sem." prefix and prepend "/"

                            // Check if the file is in use
                            if (IsFileInUse(fullPath))
                            {
                                std::cout << "Semaphore in use, skipping: " << semName << std::endl;
                                break; // Skip to next file
                            }

                            // Attempt to unlink it
                            if (sem_unlink(semName.c_str()) == 0)
                            {
                                std::cout << "Unlinked semaphore: " << semName << std::endl;
                            }
                            else
                            {
                                // Ignore errors if the file doesn't exist
                                if (errno != ENOENT)
                                {
                                    perror(("sem_unlink failed for " + semName).c_str());
                                }
                            }

                            break; // No need to check other prefixes
                        }
                    }
                }
            }

            // Close the directory
            closedir(dir);
        }


    } // namespace IPC
} // namespace X
#endif
