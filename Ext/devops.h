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
		class DebugService
		{
			bool BuildStackInfo(
				XlangRuntime* rt,
				XObj* pContextCurrent,
				CommandInfo* pCommandInfo,
				X::Value& valStackInfo);
			bool PackScopeVars(XlangRuntime* rt, 
				XObj* pContextCurrent, AST::Scope* pScope,
				X::Value& varPackList);
			bool PackScopeSpecialVars(XlangRuntime* rt,
				XObj* pContextCurrent, AST::Scope* pScope,
				X::Value& varPackList);
			bool BuildGlobals(XlangRuntime* rt,
				XObj* pContextCurrent,
				X::Value& valGlobals);
			bool BuildLocals(XlangRuntime* rt,
				XObj* pContextCurrent, AST::StackFrame* frameId,
				X::Value& valLocals);
			bool BuildObjectContent(XlangRuntime* rt,
				XObj* pContextCurrent, AST::StackFrame* frameId, X::Value& valParam,
				X::Value& valObject);
			bool ObjectSetValue(XlangRuntime* rt,
				XObj* pContextCurrent, AST::StackFrame* frameId, X::Value& valParam,
				X::Value& objRetValue);
		public:
			BEGIN_PACKAGE(DebugService)
				APISET().AddFunc<1>("get_startline", &DebugService::GetModuleStartLine);
				APISET().AddFunc<0>("get_threads", &DebugService::GetThreads);
				APISET().AddRTFunc<2>("set_breakpoints", &DebugService::SetBreakpoints);
				APISET().AddVarFunc("command", &DebugService::Command);
				APISET().AddVarFunc("run_file", &DebugService::RunFile);
				APISET().AddVarFunc("stop_file", &DebugService::StopFile);
				APISET().AddFunc<1>("set_debug", &DebugService::SetDebug);
			END_PACKAGE
			DebugService();
			int GetModuleStartLine(unsigned long long moduleKey);
			X::Value GetThreads();
			X::Value SetBreakpoints(X::XRuntime* rt,X::XObj* pContext, Value& varPath, Value& varLines);
			bool Command(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
			bool RunFile(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
			bool StopFile(X::XRuntime* rt, X::XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				X::Value& retValue);
			void SetDebug(int iVal);
		private:
			static bool s_bRegPlugins;
			static std::unordered_map<std::string, X::Value> m_mapPluginModule;
			static void registerPlugins();

			X::Value execFile(bool bRun, const std::string& filePath, X::XRuntime* rt);
		};
	}
}