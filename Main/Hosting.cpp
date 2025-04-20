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

#include "Hosting.h"
#include "parser.h"
#include "gthread.h"
#include "moduleobject.h"
#include "module.h"
#include "event.h"
#include "exp_exec.h"
#include "port.h"
#include "dbg.h"
#include "../Jit/md5.h"
#include <filesystem>


namespace X
{
	class Backend :
		public GThread
	{
		std::string m_moduleName;
		std::string m_code;
		std::vector<X::Value> m_passInParams;
		// Inherited via GThread
		virtual void run() override
		{
			X::Value retVal;
			Hosting::I().Run(m_moduleName.c_str(), m_code.c_str(),
				(int)m_code.size(), m_passInParams,retVal);
		}
	public:
		Backend(std::string& moduleName,std::string& code, std::vector<X::Value>& passInParams)
		{
			m_moduleName = moduleName;
			m_code = code;
			m_passInParams = passInParams;
		}

	};
	bool Hosting::PostRunFragmentInMainThread(AST::ModuleObject* pModuleObj, std::string& code)
	{
		X::ARGS params0(2);
		params0.push_back(X::Value(pModuleObj));
		params0.push_back(X::Value(code));
		EventSystem::I().AddEventTask([this](X::ARGS params)
			{
				auto* pModuleObj0 = dynamic_cast<AST::ModuleObject*>(params[0].GetObj());
				std::string code = params[1].ToString();
				X::Value retVal;
				RunFragmentInModule(pModuleObj0, code.c_str(), code.size(), retVal);
			}, params0);
		return true;
	}

	void Hosting::SetDebugMode(bool bDebug)
	{
		if (bDebug)
			G::I().SetTrace(Dbg::xTraceFunc);
		else
		{
			G::I().SetTrace(nullptr);
			std::unordered_map<long long, XlangRuntime*> rtMap = G::I().GetThreadRuntimeIdMap();
			for (auto& item : rtMap)
			{
				if (item.second->m_bStoped)
				{
					CommandInfo* pCmdInfo = new CommandInfo();
					pCmdInfo->dbgType = dbg::Continue;
					item.second->AddCommand(pCmdInfo, false);
				}
			}
		}
	}

	unsigned long long  Hosting::RunAsBackend(std::string& moduleName, std::string& code, std::vector<X::Value>& args)
	{
		Backend* pBackend = new Backend(moduleName,code, args);
		pBackend->Start();
		return (unsigned long long)(void*)pBackend;
	}
	AppEventCode Hosting::HandleAppEvent(int signum)
	{
		std::cout << "((c)ontinue,(b)reak,(e)xit>>";
		std::string input;
		std::getline(std::cin, input);
		if (input == "e" || input == "E")
		{
			return AppEventCode::Exit;
		}
		else if (input == "c" || input == "C")
		{
			return AppEventCode::Continue;
		}
		m_lock.Lock();
		for (int i=0;i<(int)m_Modules.size();i++)
		{
			auto m = m_Modules[i];
			std::cout << "(" << i + 1 << ")" << m->GetModuleName() << std::endl;
		}
		m_lock.Unlock();
		std::cout << "(R)esume"<<std::endl;
		std::cout << ">>";
		while (true) 
		{
			std::getline(std::cin, input);
			if (input == "R" || input == "r")
			{
				return AppEventCode::Continue;
			}
			int idx = 0;
			
			#if defined(BARE_METAL)
				idx = std::stoi(input);
			#else
				try
				{
					idx = std::stoi(input);
				}
				catch (...)
				{
					idx = -1;
				}
			#endif

			if (idx >= 1 && idx <= m_Modules.size())
			{
				auto m = m_Modules[idx - 1];
				//m->SetDbgType(dbg::Step, dbg::Step);
				break;
			}
			else
			{
				std::cout << ">>";
			}
		}
		return AppEventCode::Continue;
	}
	X::Value Hosting::NewModule()
	{
		AST::Module* pTopModule = new AST::Module();
		//pTopModule->IncRef();
		pTopModule->ScopeLayout();

		XlangRuntime* pRuntime = new XlangRuntime();
		pRuntime->SetM(pTopModule);
		pTopModule->SetRT(pRuntime);
		//todo: same thread run multiple top module?
		//deal with line below
		//G::I().BindRuntimeToThread(pRuntime);

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());

		auto* pModuleObj = new X::AST::ModuleObject(pTopModule);
		return X::Value(pModuleObj);
	}
	AST::Module* Hosting::Load(const char* moduleName,
		const char* code, int size, unsigned long long& moduleKey, const std::string& md5)
	{
		Parser parser;
		if (!parser.Init())
		{
			return nullptr;
		}
		//prepare top module for this code
		AST::Module* pTopModule = new AST::Module();
		std::filesystem::path modulePath(moduleName);
		// Check if the moduleName is not an absolute path, and resolve it
		if (!modulePath.is_absolute())
		{
			modulePath = std::filesystem::absolute(modulePath);
		}
		std::string strModuleName = modulePath.generic_string();
		parser.SetModuleName(strModuleName);
		pTopModule->SetModuleName(strModuleName);
		pTopModule->SetMd5(md5);
		pTopModule->ScopeLayout();
		parser.Compile(pTopModule,(char*)code, size);

		strModuleName = pTopModule->GetModuleName();
		std::filesystem::path pathModuleName(strModuleName);
		strModuleName = pathModuleName.generic_string();
		// if source file has breakpoint data, set breakpoint for this new created module
		std::vector<int> lines = G::I().GetBreakPointsMd5(md5);
		if (lines.size() > 0)
			SendModuleLoaded(md5, strModuleName);
		bool bValid = G::I().IsBreakpointValidMd5(md5); // Whether the source file's breakpoints have been checked 
		for (const auto& l : lines)
		{
			int al = pTopModule->SetBreakpoint(l, (int)GetThreadID());

			if (!bValid) // if source file has not been checked, return breakpoint's state to debugger
			{
				if (al >= 0)
					SendBreakpointState(md5, l, al);
				else
					SendBreakpointState(md5, l, -1); // failed state
			}
		}
		if (!bValid)
			G::I().AddBreakpointValidMd5(md5);


		moduleKey = AddModule(pTopModule);
		return pTopModule;
	}
    AST::Module *Hosting::LoadWithScope(AST::Scope *pScope, const char *code, int size)
    {
		Parser parser;
		if (!parser.Init())
		{
			return nullptr;
		}
		//prepare top module for this code
		AST::Module* pTopModule = new AST::Module();
		pTopModule->ChangeMyScopeTo(pScope);
		pTopModule->ScopeLayout();
		parser.Compile(pTopModule,(char*)code, size);
		std::string moduleName("proxy_module");
		pTopModule->SetModuleName(moduleName);
		AddModule(pTopModule);
		return pTopModule;    
	}
    bool Hosting::Unload(AST::Module *pTopModule)
    {
		RemoveModule(pTopModule);
		return true;
	}
	bool Hosting::InitRun(AST::Module* pTopModule,X::Value& retVal)
	{
		XlangRuntime* pRuntime = new XlangRuntime();
		pRuntime->SetM(pTopModule);
		pTopModule->SetRT(pRuntime);
		G::I().BindRuntimeToThread(pRuntime);

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());
		X::Value v;
		X::AST::ExecAction action;
		bool bOK = ExpExec(pTopModule,pRuntime, action,nullptr, v);
		X::Value v1 = pModuleFrame->GetReturnValue();
		if (v1.IsValid())
		{
			retVal = v1;
		}
		else
		{
			retVal = v;
		}
		return bOK;

	}
	bool Hosting::Run(AST::Module* pTopModule, X::Value& retVal,
		std::vector<X::Value>& passInParams,
		bool stopOnEntry, bool keepModuleWithRuntime, bool noDebug)
	{
		pTopModule->SetArgs(passInParams);
		std::string name("main");
		XlangRuntime* pRuntime = G::I().Threading(pTopModule->GetModuleName(),nullptr);
		if (noDebug)
		{
			pRuntime->SetNoDbg(true);
		}
		AST::Module* pOldModule = pRuntime->M(); 
		pTopModule->SetRT(pRuntime);
		pRuntime->SetM(pTopModule);
		G::I().BindRuntimeToThread(pRuntime);
				
		if (stopOnEntry || (!pRuntime->m_bNoDbg && G::I().GetTrace()))
		{
			pTopModule->SetDebug(true,pRuntime);
			if (stopOnEntry && !pOldModule)
				pRuntime->SetDbgType(X::dbg::Step, dbg::None);
			else
				pRuntime->SetDbgType(X::dbg::Continue, dbg::Continue);
		}

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());
		X::Value v;
		X::AST::ExecAction action;
		//bool bOK = X::Exp::ExpExec(pTopModule,pRuntime,action,nullptr, v);
		bool bOK = ExpExec(pTopModule, pRuntime, action, nullptr, v);
		X::Value v1 = pModuleFrame->GetReturnValue();
		if (v1.IsValid())
		{
			retVal = v1;
		}
		else
		{
			retVal = v;
		}
		pRuntime->PopFrame(); //move from line 314, we think this module's run finished, 
		//need to popup its frame,if not, will leave a dirty stack here 
		if (!keepModuleWithRuntime)
		{
			//pRuntime->PopFrame();
			if (pOldModule)
			{
				pRuntime->SetM(pOldModule);
			}
			else
			{
				delete pRuntime;
			}
		}

		return bOK;
	}
	bool Hosting::RunWithKWArgs(AST::Module* pTopModule, X::Value& retVal,
		std::vector<X::Value>& passInParams, X::KWARGS& kwargs,
		bool stopOnEntry, bool keepModuleWithRuntime, bool noDebug)
	{
		pTopModule->SetArgs(passInParams);
		for (auto& it : kwargs)
		{
			std::string name(it.key);
			pTopModule->AddModuleVariable(name, it.val);
		}
		std::string name("main");
		XlangRuntime* pRuntime = G::I().Threading(pTopModule->GetModuleName(), nullptr);
		if (noDebug)
		{
			pRuntime->SetNoDbg(true);
		}
		AST::Module* pOldModule = pRuntime->M();
		pTopModule->SetRT(pRuntime);
		pRuntime->SetM(pTopModule);
		G::I().BindRuntimeToThread(pRuntime);

		if (stopOnEntry || (!pRuntime->m_bNoDbg && G::I().GetTrace()))
		{
			pTopModule->SetDebug(true, pRuntime);
			if (stopOnEntry && !pOldModule)
				pRuntime->SetDbgType(X::dbg::Step, dbg::None);
			else
				pRuntime->SetDbgType(X::dbg::Continue, dbg::Continue);
		}

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());
		X::Value v;
		X::AST::ExecAction action;
		//bool bOK = X::Exp::ExpExec(pTopModule,pRuntime,action,nullptr, v);
		bool bOK = ExpExec(pTopModule, pRuntime, action, nullptr, v);
		X::Value v1 = pModuleFrame->GetReturnValue();
		if (v1.IsValid())
		{
			retVal = v1;
		}
		else
		{
			retVal = v;
		}
		pRuntime->PopFrame(); //move from line 314, we think this module's run finished, 
		//need to popup its frame,if not, will leave a dirty stack here 
		if (!keepModuleWithRuntime)
		{
			//pRuntime->PopFrame();
			if (pOldModule)
			{
				pRuntime->SetM(pOldModule);
			}
			else
			{
				delete pRuntime;
			}
		}

		return bOK;
	}
	bool Hosting::RunFragmentInModule(
		AST::ModuleObject* pModuleObj,
		const char* code, int size, X::Value& retVal, int exeNum /*= -1*/)
	{
		AST::Module* pModule = pModuleObj->M();
		long long lineCntBeforeAdd = pModule->GetBodySize();
		Parser parser;
		if (!parser.Init())
		{
			return false;
		}
		bool bOK = parser.Compile(pModule, (char*)code, size);
		if (!bOK)
		{
			//todo:syntax error
			return false;
		}
		m_ExeNum = exeNum;
		auto* rt = pModule->GetRT();
		if (rt)
		{
			rt->AdjustStack(pModule->GetMyScope()->GetVarNum());
		}
		bOK = pModule->RunFromLine(rt, pModuleObj, lineCntBeforeAdd,retVal);
		m_ExeNum = -1;
		return bOK;
	}
	/*
		keep a module to run lines from interactive mode
		such as commmand line input
	*/
	bool Hosting::RunCodeLine(const char* code, int size, X::Value& retVal)
	{
		if (m_pInteractiveModule == nullptr)
		{
			auto* pTopModule = new AST::Module();
			pTopModule->ScopeLayout();
			XlangRuntime* pRuntime = new XlangRuntime();
			pRuntime->SetM(pTopModule);
			G::I().BindRuntimeToThread(pRuntime);
			AST::StackFrame* pModuleFrame = pTopModule->GetStack();
			pModuleFrame->SetLine(pTopModule->GetStartLine());
			pTopModule->AddBuiltins(pRuntime);
			pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());
			m_pInteractiveModule = pTopModule;
			//m_pInteractiveModule->IncRef();
			m_pInteractiveRuntime = pRuntime;
		}
		Parser parser;
		if (!parser.Init())
		{
			return false;
		}
		bool bOK = parser.Compile(m_pInteractiveModule, (char*)code, size);
		if (!bOK)
		{
			//todo:syntax error
			return false;
		}
		m_pInteractiveRuntime->AdjustStack(m_pInteractiveModule->GetMyScope()->GetVarNum());
		bOK = m_pInteractiveModule->RunLast(m_pInteractiveRuntime, nullptr, retVal);
		return bOK;
	}
	bool Hosting::GetInteractiveCode(std::string& code)
	{
		if (m_pInteractiveModule)
		{
			code = m_pInteractiveModule->GetCode();
		}
		return (m_pInteractiveModule !=nullptr);
	}
	bool Hosting::Run(const char* moduleName,
		const char* code, int size, 
		std::vector<X::Value>& passInParams,
		X::Value& retVal,
		bool noDebug)
	{
		unsigned long long moduleKey = 0;
		AST::Module* pTopModule = Load(moduleName, code, size, moduleKey, md5(code));
		if (pTopModule == nullptr)
		{
			return false;
		}
		bool bOK =  Run(pTopModule,retVal, passInParams,false,false,true);
		Unload(pTopModule);
		return bOK;
	}

	//this SimpleRun not bind thread to runtime
	//for json parser
	bool Hosting::SimpleRun(const char* moduleName,
		const char* code, int size,
		X::Value& retVal)
	{
		unsigned long long moduleKey = 0;
		AST::Module* pTopModule = Load(moduleName, code, size, moduleKey, md5(code));
		if (pTopModule == nullptr)
		{
			return false;
		}
		XlangRuntime* pRuntime = new XlangRuntime();
		pRuntime->SetNoThreadBinding(true);
		pTopModule->SetRT(pRuntime);
		pRuntime->SetM(pTopModule);

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetMyScope()->GetVarNum());
		X::AST::ExecAction action;
		bool bOK = ExpExec(pTopModule, pRuntime, action, nullptr, retVal);
		pRuntime->PopFrame();
		delete pRuntime;

		Unload(pTopModule);
		return bOK;
	}
	bool Hosting::Run(unsigned long long moduleKey, X::KWARGS& kwParams,
		X::Value& retVal)
	{
		//check debug command
		bool stopOnEntry = false;
		std::string onFinishExpr;
		auto it = kwParams.find("stopOnEntry");
		if (it)
		{
			stopOnEntry = it->val.GetBool();
		}
		it = kwParams.find("onFinish");
		if (it)
		{
			onFinishExpr = it->val.ToString();
		}

		AST::Module* pTopModule = QueryModule(moduleKey);
		if (pTopModule == nullptr)
		{
			retVal = X::Value(false);
			return false;
		}
		std::vector<X::Value> passInParams;
		bool bOK = Run(pTopModule, retVal, passInParams,stopOnEntry);
		Unload(pTopModule);
		if (!onFinishExpr.empty())
		{
			X::Value valRet0;
			std::vector<X::Value> passInParams;
			Run("Cleanup.x", onFinishExpr.c_str(), (int)onFinishExpr.size(), passInParams,valRet0);
		}
		return bOK;
	}

	void Hosting::SendBreakpointState(const std::string& md5, int line, int actualLine)
	{
		KWARGS kwParams;
		X::Value valAction("notify");
		kwParams.Add("action", valAction);
		const int online_len = 1000;
		char strBuf[online_len];
		SPRINTF(strBuf, online_len, "[{\"BreakpointMd5\":\"%s\", \"line\":%d, \"actualLine\":%d}]", md5.c_str(), line, actualLine);
		X::Value valParam(strBuf);
		kwParams.Add("param", valParam);
		std::string evtName("devops.dbg");
		ARGS params(0);
		X::EventSystem::I().Fire(nullptr, nullptr, evtName, params, kwParams);
	}

	void Hosting::SendModuleLoaded(const std::string& md5, const std::string& path)
	{
		KWARGS kwParams;
		X::Value valAction("notify");
		kwParams.Add("action", valAction);
		const int online_len = 1000;
		char strBuf[online_len];
#ifdef _WIN32
		std::string pathU8 = systemCpToUtf8(path);
		SPRINTF(strBuf, online_len, "[{\"ModuleLoaded\":\"%s\", \"md5\":\"%s\"}]", pathU8.c_str(), md5.c_str());
#else
		SPRINTF(strBuf, online_len, "[{\"ModuleLoaded\":\"%s\", \"md5\":\"%s\"}]", path.c_str(), md5.c_str());
#endif
		X::Value valParam(strBuf);
		kwParams.Add("param", valParam);
		std::string evtName("devops.dbg");
		ARGS params(0);
		X::EventSystem::I().Fire(nullptr, nullptr, evtName, params, kwParams);
	}
}