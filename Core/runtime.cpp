#include "runtime.h"
#include "stackframe.h"
#include "exp.h"
#include "module.h"
#include "glob.h"

namespace X 
{
	XlangRuntime::~XlangRuntime()
	{
		G::I().UnbindRuntimeToThread(this);
	}
	bool XlangRuntime::CreateEmptyModule()
	{
		if (m_pModule)
		{
			delete m_pModule;
		}
		m_pModule = new AST::Module();
		m_pModule->ScopeLayout();
		AST::StackFrame* pModuleFrame = new AST::StackFrame(m_pModule);
		pModuleFrame->SetLine(m_pModule->GetStartLine());
		m_pModule->AddBuiltins(this);
		PushFrame(pModuleFrame, m_pModule->GetVarNum());

		return true;
	}
}