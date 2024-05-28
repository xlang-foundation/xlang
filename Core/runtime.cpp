#include "runtime.h"
#include "stackframe.h"
#include "exp.h"
#include "module.h"
#include "glob.h"
#include "list.h"
#include "op.h"
#include "moduleobject.h"

namespace X 
{
	XlangRuntime::~XlangRuntime()
	{
		if (!m_noThreadBinding)
		{
			G::I().UnbindRuntimeToThread(this);
		}
	}
	bool XlangRuntime::GetWritePadNum(int& count, int& dataBindingCount)
	{
		count = (int)m_WritePads.size();
		dataBindingCount = 0;
		for (int i = 0; i < count; i++)
		{
			if (m_WritePads[i].UsingDataBinding)
			{
				dataBindingCount++;
			}
		}
		return true;
	}
	bool XlangRuntime::CallWritePads(Value& fmtString, Value& bindingString,
		Value& indexOrAlias,
		std::vector<Value> Value_Bind_list)
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
		Value ValList;
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

			ARGS params(0);
			if (pad.UsingDataBinding)
			{
				params.resize(2);
				params.push_back(bindingString);
				if (ValList.IsInvalid())
				{
					Data::List* pOutList = new Data::List();
					for (auto idx : Value_Bind_list)
					{
						(*pOutList) += idx;
					}
					XObj* pObjList = dynamic_cast<XObj*>(pOutList);
					ValList = X::Value(pObjList);
				}
				params.push_back(ValList);
			}
			else
			{
				params.resize(1);
				params.push_back(fmtString);
			}
			KWARGS kwargs;
			Value retVal;
			bool bOK = pad.writePadFunc.GetObj()->Call(
				this, pad.obj.GetObj(), params, kwargs, retVal);
		}
		return true;
	}
	int XlangRuntime::PushWritePad(X::Value valObj, std::string alias)
	{
		if (!valObj.IsObject())
		{
			return -1;
		}
		//TODO: Scope Issue
		auto* pScope = dynamic_cast<AST::Scope*>(valObj.GetObj());
		if (pScope == nullptr)
		{
			return -1;
		}
		static std::string WritePadBindingFuncName("WritePadUseDataBinding");
		static std::string WritePadFuncName("WritePad");
		bool UsingDataBinding = false;
		SCOPE_FAST_CALL_AddOrGet0(index,pScope,WritePadBindingFuncName, true);
		if (index >= 0)
		{
			X::Value varFunc;
			if (!Get(pScope,nullptr, index, varFunc))
			{
				return -1;
			}
			ARGS params(0);
			KWARGS kwargs;
			Value retVal;
			bool bOK = varFunc.GetObj()->Call(this, 
				valObj.GetObj(), params, kwargs, retVal);
			if (bOK && retVal.IsTrue())
			{
				UsingDataBinding = true;
			}
		}
		SCOPE_FAST_CALL_AddOrGet0_NoDef(index,pScope,WritePadFuncName, true);
		if (index < 0)
		{
			return -1;
		}
		X::Value varFunc;
		if (!Get(pScope, nullptr, index, varFunc))
		{
			return -1;
		}
		m_WritePads.push_back(WritePadInfo{valObj,varFunc,
			UsingDataBinding,alias});
		int padIndex = (int)m_WritePads.size() - 1;
		if (!alias.empty())
		{
			m_WritePadMap.emplace(std::make_pair(alias,padIndex));
		}
		return padIndex;
	}
	void XlangRuntime::PopWritePad()
	{
		int size = (int)m_WritePads.size();
		if (size == 0)
		{
			return;
		}
		auto last = m_WritePads[size - 1];
		ARGS params(2);
		params.push_back(X::Value());//Invalid value means cleanup this pad
		if (last.UsingDataBinding)
		{
			params.push_back(X::Value());
		}
		params.Close();
		KWARGS kwargs;
		Value retVal;
		last.writePadFunc.GetObj()->Call(
			this, last.obj.GetObj(), params, kwargs, retVal);

		if (!last.alias.empty())
		{
			auto it = m_WritePadMap.find(last.alias);
			m_WritePadMap.erase(it);
		}
		m_WritePads.erase(m_WritePads.end() - 1);
	}
	bool XlangRuntime::CreateEmptyModule()
	{
		if (m_pModule)
		{
			delete m_pModule;
		}
		m_pModule = new AST::Module();
		m_pModule->ScopeLayout();
		//old code: 
		//AST::StackFrame* pModuleFrame = new AST::StackFrame(m_pModule->GetMyScope());
		//pModuleFrame->SetLine(m_pModule->GetStartLine());
		//1/6/2024 changed
		//reuse m_pModule's m_stackFrame by GetStack()
		AST::StackFrame* pModuleFrame = m_pModule->GetStack();
		m_pModule->AddBuiltins(this);
		PushFrame(pModuleFrame, m_pModule->GetMyScope()->GetVarNum());

		return true;
	}
	X::Value XlangRuntime::GetXModuleFileName()
	{
		std::string moduleFileName;
		if (m_pModule)
		{
			moduleFileName = m_pModule->GetModuleName();
		} 
		return X::Value(moduleFileName);
	}
	int XlangRuntime::GetTopStackCurrentLine()
	{
		if (m_stackBottom)
		{
			return m_stackBottom->GetStartLine();
		}
		return -1;
	}
}