﻿/*
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
#include "SwapBufferStream.h"
#include <vector>
#include <functional>
#include <string>
#if (WIN32)
#include <Windows.h>
#include <sddl.h>
#endif

namespace X
{
	namespace IPC
	{
		struct Call_Context
		{
			unsigned int reqId = 0;
		};
		class Helper
		{
		public:
			inline static bool CheckIfAdmin()
			{
#if (WIN32)
				BOOL isAdmin = FALSE;
				PSID adminGroup = NULL;

				// Create a SID for the administrators group
				SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
				if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
					return false;
				}

				// Check if the current token contains the admin SID
				if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
					isAdmin = FALSE;
				}

				FreeSid(adminGroup);
				return (isAdmin == TRUE);

#else
				//in linux, we don't need to add global as prefix for IPC objects
				//so we just return false
				return false;
#endif
			}
		};
		class RemotingProc
		{
		public:
			virtual int AddRef() = 0;
			virtual int Release() = 0;
			virtual int RefCount() = 0;
			virtual void EndReceiveCall(SwapBufferStream& stream) = 0;
			virtual SwapBufferStream& BeginWriteReturn(void* pCallContext,long long retCode) = 0;
			virtual void EndWriteReturn(void* pCallContext, long long retCode) = 0;
			virtual unsigned long long GetSessionId() = 0;
			virtual void ShakeHandsCall(void* pCallContext, SwapBufferStream& stream) = 0;
		};
		class RemotingCallBase
		{
		public:
			virtual bool Call(
				void* pCallContext,
				unsigned int callType,
				SwapBufferStream& stream,
				RemotingProc* pProc) = 0;
		};
		struct RemoteFuncInfo
		{
			unsigned int callID;
			unsigned int _align;//For 8-bytes aligned
			RemotingCallBase* pHandler;
			std::string funcName;
			std::vector<std::string> inputTypes;
			std::string retType;
		};
		using call_func = std::function<void()>;
	}
}