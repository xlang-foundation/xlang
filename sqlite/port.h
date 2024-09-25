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
