#include "Hosting.h"
#include "parser.h"
#include "gthread.h"
#include "moduleobject.h"

namespace X
{
	class Backend :
		public GThread
	{
		std::string m_moduleName;
		std::string m_code;
		// Inherited via GThread
		virtual void run() override
		{
			X::Value retVal;
			std::vector<std::string> passInParams;
			Hosting::I().Run(m_moduleName,m_code.c_str(),
				(int)m_code.size(), passInParams,retVal);
		}
	public:
		Backend(std::string& moduleName,std::string& code)
		{
			m_moduleName = moduleName;
			m_code = code;
		}

	};
	bool Hosting::RunAsBackend(std::string& moduleName, std::string& code,
		std::vector<std::string>& passInParams)
	{
		Backend* pBackend = new Backend(moduleName,code);
		pBackend->Start();
		return true;
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
			try
			{
				idx = std::stoi(input);
			}
			catch (...)
			{
				idx = -1;
			}
			if (idx >= 1 && idx <= m_Modules.size())
			{
				auto m = m_Modules[idx - 1];
				m->SetDbgType(AST::dbg::Step, AST::dbg::Step);
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
		pTopModule->IncRef();
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
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetVarNum());

		auto* pModuleObj = new X::AST::ModuleObject(pTopModule);
		return X::Value(pModuleObj);
	}
	AST::Module* Hosting::Load(std::string& moduleName,
		const char* code, int size, unsigned long long& moduleKey)
	{
		Parser parser;
		if (!parser.Init())
		{
			return nullptr;
		}
		//prepare top module for this code
		AST::Module* pTopModule = new AST::Module();
		pTopModule->IncRef();
		pTopModule->ScopeLayout();
		parser.Compile(pTopModule,(char*)code, size);
		pTopModule->SetModuleName(moduleName);
		moduleKey = AddModule(pTopModule);
		return pTopModule;
	}
	bool Hosting::Unload(AST::Module* pTopModule)
	{
		RemoveModule(pTopModule);
		pTopModule->DecRef();
		return true;
	}
	bool Hosting::InitRun(AST::Module* pTopModule,X::Value& retVal)
	{
		XlangRuntime* pRuntime = new XlangRuntime();
		pRuntime->SetM(pTopModule);
		G::I().BindRuntimeToThread(pRuntime);

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetVarNum());
		X::Value v;
		X::AST::ExecAction action;
		bool bOK = pTopModule->Exec(pRuntime, action,nullptr, v);
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
		std::vector<std::string>& passInParams,
		bool stopOnEntry)
	{
		pTopModule->SetArgs(passInParams);
		XlangRuntime* pRuntime = new XlangRuntime();
		pRuntime->SetM(pTopModule);
		G::I().BindRuntimeToThread(pRuntime);
		if (stopOnEntry)
		{
			pTopModule->SetDebug(true,pRuntime);
			pTopModule->SetDbgType(X::AST::dbg::Step,
				AST::dbg::Step);
		}

		AST::StackFrame* pModuleFrame = pTopModule->GetStack();
		pModuleFrame->SetLine(pTopModule->GetStartLine());
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetVarNum());
		X::Value v;
		X::AST::ExecAction action;
		bool bOK = pTopModule->Exec(pRuntime,action,nullptr, v);
		pRuntime->PopFrame();
		X::Value v1 = pModuleFrame->GetReturnValue();
		if (v1.IsValid())
		{
			retVal = v1;
		}
		else
		{
			retVal = v;
		}
		delete pRuntime;
		return bOK;
	}
	bool Hosting::RunFragmentInModule(
		AST::ModuleObject* pModuleObj,
		const char* code, int size, X::Value& retVal)
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
		auto* rt = pModule->GetRT();
		if (rt)
		{
			rt->AdjustStack(pModule->GetVarNum());
		}
		bOK = pModule->RunFromLine(rt, pModuleObj, lineCntBeforeAdd,retVal);
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
			pRuntime->PushFrame(pModuleFrame, pTopModule->GetVarNum());
			m_pInteractiveModule = pTopModule;
			m_pInteractiveModule->IncRef();
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
		m_pInteractiveRuntime->AdjustStack(m_pInteractiveModule->GetVarNum());
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
	bool Hosting::Run(std::string& moduleName,
		const char* code, int size, 
		std::vector<std::string>& passInParams,
		X::Value& retVal)
	{
		unsigned long long moduleKey = 0;
		AST::Module* pTopModule = Load(moduleName, code, size, moduleKey);
		if (pTopModule == nullptr)
		{
			return false;
		}
		bool bOK =  Run(pTopModule,retVal, passInParams);
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
		if (it != kwParams.end())
		{
			stopOnEntry = it->second.GetBool();
		}
		it = kwParams.find("onFinish");
		if (it != kwParams.end())
		{
			onFinishExpr = it->second.ToString();
		}

		AST::Module* pTopModule = QueryModule(moduleKey);
		if (pTopModule == nullptr)
		{
			retVal = X::Value(false);
			return false;
		}
		std::vector<std::string> passInParams;
		bool bOK = Run(pTopModule, retVal, passInParams,stopOnEntry);
		Unload(pTopModule);
		if (!onFinishExpr.empty())
		{
			std::string moduleName("Cleanup.x");
			X::Value valRet0;
			std::vector<std::string> passInParams;
			Run(moduleName, onFinishExpr.c_str(), (int)onFinishExpr.size(), passInParams,valRet0);
		}
		return bOK;
	}
}