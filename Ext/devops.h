#pragma once
#include "xpackage.h"
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
			inline virtual bool Init()
			{
				return mImpl->Init();
			}
			//must be virtual
			inline virtual bool Uninit()
			{
				return mImpl->Uninit();
			}
		};
		class DebugService
		{
			bool BuildStackInfo(
				Runtime* rt,
				void* pContextCurrent,
				AST::CommandInfo* pCommandInfo,
				AST::Value& valStackInfo);
			bool BuildLocals(Runtime* rt,
				void* pContextCurrent,int frameId,
				AST::Value& valLocals);
			bool BuildObjectContent(Runtime* rt,
				void* pContextCurrent, int frameId, AST::Value& valParam,
				AST::Value& valObject);
		public:
			BEGIN_PACKAGE(DebugService)
				ADD_FUNC("get_startline", GetModuleStartLine)
				ADD_FUNC("set_breakpoints", SetBreakpoints)
				ADD_FUNC("command", Command)
			END_PACKAGE
			bool GetModuleStartLine(void* rt, void* pContext,
					ARGS& params,
					KWARGS& kwParams,
					AST::Value& retValue);
			bool SetBreakpoints(void* rt, void* pContext,
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