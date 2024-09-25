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

#include "devsrv.h"
#include "xhost.h"

#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

namespace X
{
	XHost* g_pXHost = nullptr;
	X::DevServer* g_pDevOps = nullptr;
}
extern "C"  X_EXPORT void Load(void* pHost,int port)
{
	X::g_pXHost = (X::XHost*)pHost;
	X::g_pDevOps = new X::DevServer(port);
	X::g_pDevOps->Start();
}
extern "C"  X_EXPORT void Unload()
{
	if (X::g_pDevOps)
	{
		X::g_pDevOps->Stop();
		delete X::g_pDevOps;
	}
	X::g_pXHost = nullptr;
}