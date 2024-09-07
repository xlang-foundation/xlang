#pragma once
#include "SwapBufferStream.h"
#include <vector>
#include <functional>
#include <string>

namespace X
{
	namespace IPC
	{
		class RemotingProc
		{
		public:
			virtual int AddRef() = 0;
			virtual int Release() = 0;
			virtual void EndReceiveCall(SwapBufferStream& stream) = 0;
			virtual void BeginWriteReturn(SwapBufferStream& stream, bool callIsOk) = 0;
			virtual void EndWriteReturn(void* pCallContext, SwapBufferStream& stream, bool callIsOk) = 0;
			virtual unsigned long long GetSessionId() = 0;
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