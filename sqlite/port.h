#if (WIN32)
#include <Windows.h>
#define Path_Sep_S "\\"
#define Path_Sep '\\'
#define SPRINTF sprintf_s
#define SCANF sscanf_s
#define MS_SLEEP(t) Sleep(t)
#define US_SLEEP(t) Sleep(t/1000)

#else
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <dlfcn.h>
#define Path_Sep_S "/"
#define Path_Sep '/'
#define SPRINTF snprintf
#define SCANF sscanf
#define MS_SLEEP(t)  usleep((t)*1000)
#define US_SLEEP(t) usleep(t)
#endif
