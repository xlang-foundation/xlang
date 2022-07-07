#include "Hosting.h"
#include "parser.h"
#include "gthread.h"

namespace X
{
	class Backend :
		public GThread
	{
		std::string m_moduleName;
		char* m_code = nullptr;
		int m_codeSize = 0;
		// Inherited via GThread
		virtual void run() override
		{
			AST::Value retVal;
			Hosting::I().Run(m_moduleName,m_code, m_codeSize,retVal);
		}
	public:
		Backend(std::string& moduleName,const char* code, int size)
		{
			m_moduleName = moduleName;
			m_code = (char*)code;
			m_codeSize = size;
		}

	};
	bool Hosting::RunAsBackend(std::string& moduleName,const char* code, int size)
	{
		Backend* pBackend = new Backend(moduleName,code, size);
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
				m->SetDbg(AST::dbg::Step);
				break;
			}
			else
			{
				std::cout << ">>";
			}
		}
		return AppEventCode::Continue;
	}
	AST::Module* Hosting::Load(std::string& moduleName,
		const char* code, int size, unsigned long long& moduleKey)
	{
		Parser parser;
		if (!parser.Init())
		{
			return nullptr;
		}
		parser.Compile((char*)code, size);
		AST::Module* pTopModule = parser.GetModule();
		pTopModule->SetModuleName(moduleName);
		moduleKey = AddModule(pTopModule);
		return pTopModule;
	}
	bool Hosting::Unload(AST::Module* pTopModule)
	{
		RemoveModule(pTopModule);
		delete pTopModule;
		return true;
	}
	bool Hosting::Run(AST::Module* pTopModule, AST::Value& retVal)
	{
		Runtime* pRuntime = new Runtime();
		pRuntime->SetM(pTopModule);
		AST::StackFrame* pModuleFrame = new AST::StackFrame(pTopModule);
		pTopModule->AddBuiltins(pRuntime);
		pRuntime->PushFrame(pModuleFrame, pTopModule->GetVarNum());
		AST::Value v;
		bool bOK = pTopModule->Run(pRuntime, nullptr, v);
		pRuntime->PopFrame();
		retVal = pModuleFrame->GetReturnValue();
		delete pModuleFrame;
		delete pRuntime;
		return bOK;
	}
	bool Hosting::Run(std::string& moduleName,
		const char* code, int size, AST::Value& retVal)
	{
		unsigned long long moduleKey = 0;
		AST::Module* pTopModule = Load(moduleName, code, size, moduleKey);
		if (pTopModule == nullptr)
		{
			return false;
		}
		bool bOK =  Run(pTopModule, retVal);
		Unload(pTopModule);
		return bOK;
	}
	bool Hosting::Run(unsigned long long moduleKey, X::KWARGS& kwParams,
		AST::Value& retVal)
	{
		//check debug command
		bool stopOnEntry = false;
		auto it = kwParams.find("stopOnEntry");
		if (it != kwParams.end())
		{
			stopOnEntry = it->second.GetBool();
		}

		AST::Module* pTopModule = QueryModule(moduleKey);
		if (pTopModule == nullptr)
		{
			retVal = X::AST::Value(false);
			return false;
		}
		if (stopOnEntry)
		{
			pTopModule->SetDbg(X::AST::dbg::Step);
		}
		bool bOK = Run(pTopModule, retVal);
		Unload(pTopModule);
	}
}