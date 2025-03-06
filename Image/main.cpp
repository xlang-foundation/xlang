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

#include "xhost.h"
#include "xpackage.h"
#include "xlFactory.h"
#include "utility.h"

#if (WIN32)
#define X_EXPORT __declspec(dllexport) 
#else
#define X_EXPORT
#endif

namespace X
{
	XHost* g_pXHost = nullptr;
}

extern "C"  X_EXPORT void Load(void* pHost,X::Value curModule)
{
	std::string strFullPath;
	std::string strFolderPath;
	std::string strLibName;
	GetCurLibInfo((void*)Load, strFullPath, strFolderPath, strLibName);

	X::g_pXHost = (X::XHost*)pHost;
	X::Images::Factory::I().SetModule(curModule);
	X::RegisterPackage<X::Images::Factory>(strFullPath.c_str(),"Factory");
}
extern "C"  X_EXPORT void Unload()
{
	X::g_pXHost = nullptr;
}