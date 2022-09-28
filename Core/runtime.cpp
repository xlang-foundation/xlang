#include "runtime.h"
#include "stackframe.h"
#include "exp.h"
#include "module.h"

namespace X 
{
	bool Runtime::CreateEmptyModule()
	{
		if (m_pModule)
		{
			delete m_pModule;
		}
		m_pModule = new AST::Module();
		return true;
	}
}