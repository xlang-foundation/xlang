#include "runtime.h"
#include "stackframe.h"
#include "exp.h"
#include "module.h"
#include "glob.h"
#include "list.h"

namespace X 
{
	XlangRuntime::~XlangRuntime()
	{
		G::I().UnbindRuntimeToThread(this);
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

			ARGS params;
			if (pad.UsingDataBinding)
			{
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
		auto* pScope = dynamic_cast<AST::Scope*>(valObj.GetObj());
		if (pScope == nullptr)
		{
			return -1;
		}
		static std::string WritePadBindingFuncName("WritePadUseDataBinding");
		static std::string WritePadFuncName("WritePad");
		bool UsingDataBinding = false;
		int index = pScope->AddOrGet(WritePadBindingFuncName, true);
		if (index >= 0)
		{
			X::Value varFunc;
			if (!pScope->Get(this, nullptr, index, varFunc))
			{
				return -1;
			}
			ARGS params;
			KWARGS kwargs;
			Value retVal;
			bool bOK = varFunc.GetObj()->Call(this, 
				valObj.GetObj(), params, kwargs, retVal);
			if (bOK && retVal.IsTrue())
			{
				UsingDataBinding = true;
			}
		}
		index = pScope->AddOrGet(WritePadFuncName, true);
		if (index < 0)
		{
			return -1;
		}
		X::Value varFunc;
		if (!pScope->Get(this, nullptr, index, varFunc))
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
		ARGS params;
		params.push_back(X::Value());//Invalid value means cleanup this pad
		if (last.UsingDataBinding)
		{
			params.push_back(X::Value());
		}
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
		AST::StackFrame* pModuleFrame = new AST::StackFrame(m_pModule);
		pModuleFrame->SetLine(m_pModule->GetStartLine());
		m_pModule->AddBuiltins(this);
		PushFrame(pModuleFrame, m_pModule->GetVarNum());

		return true;
	}
}