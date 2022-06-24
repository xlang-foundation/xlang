#include "Hosting.h"
#include "parser.h"
#include "gthread.h"

namespace X
{
	class Backend :
		public GThread
	{
		char* m_code = nullptr;
		int m_codeSize = 0;
		// Inherited via GThread
		virtual void run() override
		{
			Hosting::I().Run(m_code, m_codeSize);
		}
	public:
		Backend(const char* code, int size)
		{
			m_code = (char*)code;
			m_codeSize = size;
		}

	};
	bool Hosting::RunAsBackend(const char* code, int size)
	{
		Backend* pBackend = new Backend(code, size);
		pBackend->Start();
		return true;
	}
	bool Hosting::Run(const char* code, int size)
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
		Runtime* pRuntime = new Runtime();
		pRuntime->SetM(pTopModule);
		pTopModule->AddBuiltins(pRuntime);
		AST::Value v;
		bool bOK = pTopModule->Run(pRuntime, nullptr, v);
		delete pTopModule;
		delete pRuntime;

		return bOK;
	}
}