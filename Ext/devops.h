#pragma once
#include "xlang.h"
#include "function.h"
#include "runtime.h"
#include <vector>
#include <iostream>
namespace X
{
	/*
		About this design pattern
		from assmeblly code,Start function inside mImpl
		will be inlined in released version with optimization 
	*/
	namespace DevOps
	{
		class Debugger
		{
			Debugger* mImpl = nullptr;
		public:
			Debugger();
			inline Debugger(int) {}//avoid recursive constructor calls
			~Debugger();
			//must be virtual
			inline virtual bool Start()
			{
				return mImpl->Start();
			}
			//must be virtual
			inline virtual bool Stop()
			{
				return mImpl->Stop();
			}
		};
		class DebugService
		{
			bool BuildStackInfo(Runtime* rt,AST::Expression* pCurExp,
				AST::Value& valStackInfo);
		public:
			BEGIN_PACKAGE(DebugService)
				ADD_FUNC("get_startline", GetModuleStartLine)
				ADD_FUNC("command", Command)
			END_PACKAGE
			bool GetModuleStartLine(void* rt, void* pContext,
					ARGS& params,
					KWARGS& kwParams,
					AST::Value& retValue);
			bool Command(void* rt, void* pContext,
				ARGS& params,
				KWARGS& kwParams,
				AST::Value& retValue);
		};
	}
}