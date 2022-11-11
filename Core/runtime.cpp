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
	bool XlangRuntime::CallWritePads(Value& input, Value& indexOrAlias)
	{
		int padIndex = -1;
		std::string alias;
		if (indexOrAlias.GetType() == ValueType::Int64)
		{
			padIndex = (int)indexOrAlias;
		}
		else if (indexOrAlias.IsObject() 
			&& indexOrAlias.GetObj()->GetType() == ObjType::Str)
		{
			alias = indexOrAlias.ToString();
		}
		for (int i=0;i<(int)m_WritePads.size();i++)
		{
			auto& pad = m_WritePads[i];
			if (padIndex >= 0)
			{
				if (i != padIndex)
				{
					continue;
				}
			}
			else if (!alias.empty() && pad.alias != alias)
			{
				continue;
			}

			ARGS params;
			params.push_back(input);
			KWARGS kwargs;
			Value retVal;
			bool bOK = pad.writePadFunc.GetObj()->Call(this, pad.obj.GetObj(), params, kwargs, retVal);
		}
		return true;
	}
	int XlangRuntime::PushWritePad(X::Value valObj, std::string alias)
	{
		if (!valObj.IsObject())
		{
			return -1;
		}
		auto* pScope = dynamic_cast<AST::Scope*>(valObj.GetObj());
		if (pScope == nullptr)
		{
			return -1;
		}
		static std::string WritePadFuncName("WritePad");
		int index = pScope->AddOrGet(WritePadFuncName, true);
		if (index < 0)
		{
			return -1;
		}
		X::Value varFunc;
		if (!pScope->Get(this, nullptr, index, varFunc))
		{
			return -1;
		}
		m_WritePads.push_back(WritePadInfo{valObj,varFunc,alias});
		int padIndex = (int)m_WritePads.size() - 1;
		if (!alias.empty())
		{
			m_WritePadMap.emplace(std::make_pair(alias,padIndex));
		}
		return padIndex;
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