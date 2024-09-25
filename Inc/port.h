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

#ifndef Platform_H
#define Platform_H

#if defined(WIN32)
#include <Windows.h>
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#define LibPrefix ""
#define ShareLibExt ".dll"
#define LOADLIB(path) LoadLibraryEx(path,NULL,LOAD_WITH_ALTERED_SEARCH_PATH)
#define GetProc(handle,funcName) GetProcAddress((HMODULE)handle, funcName)
#define UNLOADLIB(h) FreeLibrary((HMODULE)h)

#define SPRINTF sprintf_s
#define SCANF sscanf_s
#define MS_SLEEP(t) Sleep(t)
#define US_SLEEP(t) Sleep(t/1000)
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

#elif defined(BARE_METAL)
// Bare metal does not support these features, use empty implementations or placeholders
#define Path_Sep_S "/"
#define Path_Sep '/'
#define LibPrefix ""
#define ShareLibExt ".lib" // No shared libraries in bare metal, use a placeholder
#define LOADLIB(path) nullptr // No dynamic loading in bare metal
#define GetProc(handle,funcName) nullptr // No dynamic loading in bare metal
#define UNLOADLIB(handle) // No dynamic loading in bare metal

#define SPRINTF snprintf
#define SCANF sscanf
#define MS_SLEEP(t) // No sleep in bare metal
#define US_SLEEP(t) // No sleep in bare metal

#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <strings.h>

#define Path_Sep_S "/"
#define Path_Sep '/'
#define LibPrefix "lib"
#if defined(__APPLE__)
#define ShareLibExt ".dylib"
#else
#define ShareLibExt ".so"
#endif
#define LOADLIB(path) dlopen(path, RTLD_LAZY)
#define GetProc(handle,funcName) dlsym(handle, funcName)
#define UNLOADLIB(handle) dlclose(handle)

#define SPRINTF snprintf
#define SCANF sscanf
#define MS_SLEEP(t)  usleep((t)*1000)
#define US_SLEEP(t) usleep(t)
#endif

#define LIST_PASS_PROCESS_SIZE 100

#if defined(WIN32)
#include <windows.h>
#define SEMAPHORE_HANDLE HANDLE
#define CREATE_SEMAPHORE(sa,name) CreateEvent(&sa, FALSE,FALSE, name)
#define OPEN_SEMAPHORE(name) OpenEvent(EVENT_ALL_ACCESS, FALSE, name)
#define WAIT_FOR_SEMAPHORE(handle, timeout) WaitForSingleObject(handle, timeout)
#define CLOSE_SEMAPHORE(handle) CloseHandle(handle)

#elif defined(BARE_METAL)
// Bare metal does not support semaphores, use empty implementations or placeholders
#define SEMAPHORE_HANDLE void*
#define CREATE_SEMAPHORE(sa,name) nullptr // No semaphores in bare metal
#define OPEN_SEMAPHORE(name) nullptr // No semaphores in bare metal
#define WAIT_FOR_SEMAPHORE(handle, timeout) // No semaphores in bare metal
#define CLOSE_SEMAPHORE(handle) // No semaphores in bare metal

#else
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#define SEMAPHORE_HANDLE sem_t*
#define CREATE_SEMAPHORE(sa,name) sem_open(name, O_CREAT | O_EXCL, 0644, 1)
#define OPEN_SEMAPHORE(name) sem_open(name, 0)
#define WAIT_FOR_SEMAPHORE(handle, timeout) sem_wait(handle) // Implement timeout if needed
#define CLOSE_SEMAPHORE(handle) sem_close(handle)
#endif

#endif // Platform_H
