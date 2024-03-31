#if (WIN32)
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
#define ShareLibExt ".so"
#define LOADLIB(path) dlopen(path, RTLD_LAZY)
#define GetProc(handle,funcName) dlsym(handle, funcName)
#define UNLOADLIB(handle) dlclose(handle)

#define SPRINTF snprintf
#define SCANF sscanf
#define MS_SLEEP(t)  usleep((t)*1000)
#define US_SLEEP(t) usleep(t)
#endif


#define LIST_PASS_PROCESS_SIZE 100


#if (WIN32)
#include <windows.h>
#define SEMAPHORE_HANDLE HANDLE
#define CREATE_SEMAPHORE(sa,name) CreateEvent(&sa, FALSE,FALSE, name)
#define OPEN_SEMAPHORE(name) OpenEvent(EVENT_ALL_ACCESS, FALSE, name)
#define WAIT_FOR_SEMAPHORE(handle, timeout) WaitForSingleObject(handle, timeout)
#define CLOSE_SEMAPHORE(handle) CloseHandle(handle)
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
