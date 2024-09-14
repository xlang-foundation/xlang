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
				BOOL isAdmin = false;
#if (WIN32)
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
#else
				//in linux, we don't need to add global as prefix for IPC objects
				//so we just return false
#endif
				return isAdmin;
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