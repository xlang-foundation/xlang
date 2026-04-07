/*
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

#include "func.h"
#include "object.h"
#include "function.h"
#include <string>
#include "glob.h"
#include "xclass_object.h"
#include "attribute.h"


namespace X
{
namespace AST
{
void Func::ScopeLayout()
{
	if (m_PassScopeLayout)
	{
		return;
	}
	m_PassScopeLayout = true;
	std::string thisKey("this");
	Scope* pMyScope = GetScope();
	//for lambda function, no Name,so skip it when m_Name.size ==0
	if (pMyScope && m_Name.size > 0)
	{
		std::string strName(m_Name.s, m_Name.size);
		SCOPE_FAST_CALL_AddOrGet0_NoDef(m_Index,pMyScope,strName, false);
		//if m_parent is calss
		// with check m_parent->m_type == ObType::Class
		// add this into myscope with index m_IndexOfThis
		//but if function is not belong to class, also add thin index
		//but it point to function itself
		if (m_parent->m_type == ObType::Class)
		{
			SCOPE_FAST_CALL_AddOrGet0_NoDef(m_IndexOfThis, GetMyScope(), thisKey, false);
		}
	}
	//process parameters' default values
	// Clear for idempotent re-entry (lambda functions may re-run ScopeLayout)
	m_DefaultExprs.clear();
	if (Params)
	{
		auto& list = Params->GetList();
		//this case happened in lambda function
		for (auto i : list)
		{
			std::string strVarName;
			std::string strVarType;
			Value defaultValue;
			Expression* defaultExpr = nullptr; // raw AST node for default value
			switch (i->m_type)
			{
			case ObType::Var:
			{
				Var* varName = dynamic_cast<Var*>(i);
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				// no default
			}
			break;
			case ObType::Assign:
			{
				Assign* assign = dynamic_cast<Assign*>(i);
				Var* varName = dynamic_cast<Var*>(assign->GetL());
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				defaultExpr = assign->GetR(); // raw AST node; Func owns the Assign, so lifetime is safe
			}
			break;
			case ObType::Param:
			{
				Param* param = dynamic_cast<Param*>(i);
				param->Parse(strVarName, strVarType, defaultValue);
				// For Param (name:type=val), extract the default expression from the Assign inside Type
				Expression* typeCombine = param->GetType();
				if (typeCombine && typeCombine->m_type == ObType::Assign)
				{
					Assign* assign = dynamic_cast<Assign*>(typeCombine);
					if (assign) defaultExpr = assign->GetR();
				}
			}
			break;
			}
			SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strVarName, false);
			m_IndexofParamList.push_back(idx);
			m_DefaultExprs.push_back(defaultExpr); // nullptr if no default
		}
		Params->ScopeLayout();
	}
}
std::string Func::getcode(bool includeHead)
{
	int startPos = m_charStart;
	int endPos = m_charEnd;
	int firstLineCharOff = 0;
	if (!includeHead && Body.size() >0)
	{
		auto& firstLine = Body[0];
		startPos = firstLine->GetCharStart();
		firstLineCharOff = firstLine->GetCharPos();
	}
	std::string code;
	Module* pMyModule = nil;
	Expression* pa = m_parent;
	while (pa != nil)
	{
		if (pa->m_type == ObType::Module)
		{
			pMyModule = dynamic_cast<Module*>(pa);
			break;
		}
		pa = pa->GetParent();
	}
	if (pMyModule)
	{
		code =  pMyModule->GetCodeFragment(startPos, endPos);
		if (!includeHead)
		{
			auto lines = split(code, '\n',false);
			code = "";
			if (lines.size() > 0)
			{
				code = lines[0];
			}
			for (int i=1;i<lines.size();i++)
			{
				auto& l = lines[i];
				code += '\n'+l.substr(firstLineCharOff);
			}
		}
	}
	return code;
}
bool Func::Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
{
	//when build from stream, n_Index stored into Stream,but m_scope is not
	if (m_Index == -1 || m_scope == nullptr)
	{
		ScopeLayout();
		//for lambda function, no name, so skip this check
		if (m_Name.size >0 && m_Index == -1)
		{
			return false;
		}
	}
	Data::Function* f = new Data::Function(this);
	//owned by Block
	Value v0(f);//hold one refcount
	XObj* pPassContext = f;
	for(auto it = m_decors.rbegin(); it != m_decors.rend(); ++it)
	{
		X::Value retVal;
		ExecAction action;
		ExpExec(*it,rt, action,pPassContext, retVal);
		if (retVal.IsObject())
		{
			auto* pWrapper = dynamic_cast<X::Data::Object*>(retVal.GetObj());
			if (pWrapper && pPassContext)
			{
				// Explicitly link the decorator chain
				auto* aBag = pWrapper->GetAttrBag();
				if (aBag)
				{
					// Set "origin" attribute for the decorator wrapper
					X::Value passIn(pPassContext);
					aBag->Set("origin", passIn);
					//aBag->Set("origin", v0);
				}
			}

			pPassContext = pWrapper;
			v0 = retVal;
		}
	}
	if (m_Index >= 0)
	{//Lambda doesn't need to register it, which doesn't have a name
		m_scope->Set(rt, pContext, m_Index, v0);
	}
	v = v0;
	return true;
}
//this function seems just called from Decorator::Exec
//but maybe have some problems with remoting
//todo: need to check out
bool Func::CallEx(XRuntime* rt, 
	XObj* pThis,
	XObj* pContext,
	ARGS& params,
	KWARGS& kwParams,
	X::Value& trailer,
	X::Value& retValue)
{
	kwParams.Add("origin", trailer);
	return Call(rt,pThis,pContext,params,kwParams,retValue);
}
void Func::FindMyModule()
{
	Module* myModule = nullptr;
	auto pa = m_parent;
	while (pa)
	{
		if (pa->m_type == ObType::Module)
		{
			myModule =  dynamic_cast<Module*>(pa);
			break;
		}
		else
		{
			pa = pa->GetParent();
		}
	}
	m_myModule = myModule;
}

void Func::ChangeStatmentsIntoTranslateMode(
	bool changeIfStatment, 
	bool changeLoopStatment)
{
	auto bodySize = Body.size();
	if (bodySize == 0)
	{
		return;
	}
	for (size_t idx = 0; idx < bodySize; idx++)
	{
		auto& i = Body[idx];
		if (changeIfStatment)
		{
			if (i->m_type == ObType::If)
			{
				If* pIf = dynamic_cast<If*>(i);
				if (pIf)
				{
					pIf->SetTranslateMode(true);
				}
			}
		}
		//TODO: if need add loop statment such for/while etc
	}
}

//Shawn@12/8/2023, if rt0 is nullptr,called from non-main thread,
//need to get rt from the module
//also check if in this is in-trace or not,
//if in trace, need to add Scope into trace list
bool Func::Call(XRuntime* rt0,
	XObj* pThis,
	XObj* pContext,
	ARGS& params,
	KWARGS& kwParams,
	Value& retValue)
{
	auto* rt_from = (XlangRuntime*)rt0;
	std::string name = GetNameString();
	bool newCreatedRuntime = false;
	XlangRuntime* rt = G::I().Threading(name,rt_from,newCreatedRuntime);
	auto oldModule = rt->M();

	if (!rt->M())
	{
		rt->SetM(GetMyModule());
	}
	
	if (G::I().GetTrace())
	{
		rt->M()->AddDbgScope(m_pMyScope);
	}

	auto* pContextObj = dynamic_cast<X::Data::Object*>(pContext);
	StackFrame* pCurFrame = new StackFrame(m_pMyScope);
	for (auto& kw : kwParams)
	{
		std::string strKey(kw.key);
		SCOPE_FAST_CALL_AddOrGet0_NoRet(m_pMyScope,strKey, false);
	}
	rt->PushFrame(pCurFrame,m_pMyScope->GetVarNum());
	//for Class,Add this if This is not null
	//Shawn 10/10/2025, for function we also want to support this
	//TODO: this may have some problem, so comment out for function
	// for function,we are going to add closure support
	//so comment out the check of pContextObj->GetType() == X::ObjType::XClassObject
	if (m_IndexOfThis >=0)
	{
		Value varThis;
		if (pContextObj && pContextObj->GetType() == X::ObjType::XClassObject)
		{
			//for class's function,this always point to class instance
			varThis = X::Value(pContextObj);
		}
#if 0
		else if(pThis)
		{
			varThis = X::Value(dynamic_cast<Data::Object*>(pThis));
		}
#endif
		pCurFrame->Set(m_IndexOfThis, varThis);
	}

	int num = (int)params.size();
	int indexNum = (int)m_IndexofParamList.size();
	if (num > indexNum)
	{
		num = indexNum;
	}
	for (int i = 0; i < num; i++)
	{
		pCurFrame->Set(m_IndexofParamList[i], params[i]);
	}
	// Apply default values for parameters not supplied by the caller
	if (num < indexNum)
	{
		int defaultCount = (int)m_DefaultExprs.size();
		for (int i = num; i < indexNum; i++)
		{
			if (i < defaultCount && m_DefaultExprs[i] != nullptr)
			{
				X::Value defVal;
				ExecAction defAction;
				ExpExec(m_DefaultExprs[i], rt, defAction, pContext, defVal);
				pCurFrame->Set(m_IndexofParamList[i], defVal);
			}
		}
	}
	for (auto& kw : kwParams)
	{
		std::string strKey(kw.key);
		SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strKey, false);
		if (idx >= 0)
		{
			pCurFrame->Set(idx, kw.val);
		}
	}
	Value v0;
	ExecAction action;
	Block::Exec(rt,action,pContext, v0);
	if (action.type == ExecActionType::Throw)
	{
		rt->SetException(action.exceptionValue);
	}
	rt->PopFrame();
	retValue = pCurFrame->GetReturnValue();
	delete pCurFrame;
	if (G::I().GetTrace() && rt->M())
	{
		rt->M()->RemoveDbgScope(m_pMyScope);
	}
	if (newCreatedRuntime)
	{
		delete rt;
	}
	return true;
}
XObj* ExternFunc::GetRightContextForClass(XObj* pContext)
{
	auto* pXClassObject = dynamic_cast<Data::XClassObject*>(pContext);
	return pXClassObject->QueryBaseObjForPackage(m_pContext);
}
}
}