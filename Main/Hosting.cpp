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
			Hosting::I().Run(m_moduleName,m_code, m_codeSize);
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
	bool Hosting::Run(std::string& moduleName, const char* code, int size)
	{
		Parser parser;
		if (!parser.Init())
		{
			return false;
		}
		parser.Compile((char*)code, size);
		AST::Module* pTopModule = parser.GetModule();
		if (pTopModule == nullptr)
		{
			return false;
		}
		pTopModule->SetModuleName(moduleName);
		AddModule(pTopModule);
		Runtime* pRuntime = new Runtime();
		pRuntime->SetM(pTopModule);
		pTopModule->AddBuiltins(pRuntime);
		AST::Value v;
		bool bOK = pTopModule->Run(pRuntime, nullptr, v);
		RemoveModule(pTopModule);
		delete pTopModule;
		delete pRuntime;

		return bOK;
	}
}