#include "var.h"
#include "value.h"
#include "object.h"
#include "function.h"
#include "prop.h"
#include <iostream>
#include "module.h"

namespace X
{
namespace AST
{
	enum class ScopeType
	{
		Module,
		Class,
		Func
	};
	void Var::EncodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Value v0;
		if (!Run(rt, pContext, v0))
		{
			return;
		}
		ScopeType st = ScopeType::Module;
		Scope* thisScope = m_scope;
		//code scope ID for each Var
		//if this scope is a top module, code the scopeid as 0,
		//then in decoding stage, will use top module to replace it
		if (dynamic_cast<Module*>(thisScope))
		{
			st = ScopeType::Module;
			if (thisScope->GetParentScope() == nullptr)
			{
				thisScope = nullptr;
			}
		}
		else if(dynamic_cast<XClass*>(thisScope))
		{
			st = ScopeType::Class;
		}
		else if (dynamic_cast<Func*>(thisScope))
		{
			st = ScopeType::Func;
		}
		stream << st;
		stream << (unsigned long long)(void*)thisScope;
		unsigned long long id = 0;
		bool isExternFunc = false;
		if (v0.IsObject())
		{
			XObj* pObj = v0.GetObj();
			if (pObj->GetType() == ObjType::Function)
			{
				auto* pFuncObj = dynamic_cast<Data::Function*>(pObj);
				if (pFuncObj)
				{
					auto* pFunc = pFuncObj->GetFunc();
					if (pFunc->m_type == ObType::BuiltinFunc)
					{
						isExternFunc = true;
					}
				}
			}
			if (!isExternFunc)
			{
				id = (unsigned long long)pObj;
			}
		}
		stream << id;// decoding stage, if Zero, treat as value not object
		stream << isExternFunc;
		if (id ==0)
		{
			if (!isExternFunc)
			{
				stream << v0;
			}
		}
		else
		{
			XObj* pObj = v0.GetObj();
			auto* addr = stream.ScopeSpace().Query(id);
			if (addr == nullptr)
			{//not found, dump this object and add into ScopeSpace
				stream << true;//means:object dump here
				stream << v0;
				stream.ScopeSpace().Add(id, pObj);
			}
			else
			{
				stream << false;//no object here, need to use ID to lookup from ScopeSpace
			}
		}
	}
	void Var::DecodeExtern(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		ScopeType st;
		stream >> st;
		unsigned long long scopeId = 0;
		stream >> scopeId;
		Scope* pCurScope = nullptr;
		void* pScopeAddr = stream.ScopeSpace().Query(scopeId);
		if (pScopeAddr == nullptr)
		{
			switch (st)
			{
			case ScopeType::Module:
			{
				if (scopeId == 0)
				{
					if (rt)
					{
						pCurScope = dynamic_cast<Scope*>(rt->M());
					}
				}
				else
				{//todo: need to do more to recove sub-module
					AST::Module* pModule = new AST::Module();
					pModule->ScopeLayout();
					pCurScope = dynamic_cast<Scope*>(pModule);
				}
			}
				break;
			case ScopeType::Class:
				pCurScope = dynamic_cast<Scope*>(new XClass());
				break;
			case ScopeType::Func:
				pCurScope = dynamic_cast<Scope*>(new Func());
				break;
			default:
				break;
			}
			if (scopeId != 0)
			{
				stream.ScopeSpace().Add(scopeId, pCurScope);
			}
		}
		else
		{
			pCurScope = (Scope*)pScopeAddr;
		}
		unsigned long long objid = 0;
		stream >> objid;
		bool isExternFunc = false;
		stream >> isExternFunc;
		std::string varName = GetNameString();
		Value v0;
		if (objid == 0)
		{
			if (isExternFunc)
			{
				m_scope = pCurScope;
				ScopeLayout();//find the index
			}
			else
			{
				stream >> v0;
				if (pCurScope)
				{
					pCurScope->AddAndSet(rt, pContext, varName, v0);
				}
			}
		}
		else
		{
			bool bObjEmbedHere = false;
			stream >> bObjEmbedHere;
			if (bObjEmbedHere)
			{
				stream >> v0;
				if (pCurScope)
				{
					pCurScope->AddAndSet(rt, pContext, varName, v0);
				}
			}
		}
		m_scope = pCurScope;
	}
	bool Var::ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Expression::ToBytes(rt, pContext, stream);
		stream << Name.size;
		if (Name.size > 0)
		{
			stream.append(Name.s, Name.size);
		}
		if (Index == -1 || m_scope == nullptr)
		{
			ScopeLayout();
		}
		stream << Index;
		//if Index is still -1, this case
		//will happen in decor function
		//just treat as local var
		bool isExtern = (Index !=-1) && (m_scope != stream.ScopeSpace().GetCurrentScope());
		stream << isExtern;
		//check the value if it is external
		if (isExtern)
		{
			EncodeExtern(rt, pContext, stream);
		}
		return true;
	}

	bool Var::FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> Name.size;
		if (Name.size > 0)
		{
			Name.s = new char[Name.size];
			m_needRelease = true;
			stream.CopyTo(Name.s, Name.size);
		}
		else
		{
			Name.s = nullptr;
		}
		stream >> Index;
		bool isExtern = false;
		stream >> isExtern;
		if (isExtern)
		{
			DecodeExtern(stream.ScopeSpace().RT(),stream.ScopeSpace().Context(), stream);
		}
		return true;
	}
	bool Var::GetPropValue(XlangRuntime* rt, XObj* pContext,XObj* pObj, Value& val)
	{
		bool bOK = false;
		auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
		if (pPropObj)
		{
			bOK = pPropObj->GetPropValue(rt, pContext, val);
		}
		return bOK;
	}
bool Var::CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables)
{
	Value val;
	bool bOK = Run(rt, pContext, val);
	if (bOK && val.IsObject())
	{
		Data::Object* pObj = dynamic_cast<Data::Object*>(val.GetObj());
		bOK = pObj->CalcCallables(rt, pContext, callables);
	}
	return bOK;
}
void Var::ScopeLayout(std::vector<AST::Scope*>& candidates)
{
	bool matched = false;
	if (m_scope && Index != -1)
	{//check if matche with one candidate
		for (auto it : candidates)
		{
			Scope* s = dynamic_cast<Scope*>(it);
			if (s == m_scope)
			{
				matched = true;
				break;
			}
		}
	}
	if (matched)
	{
		return;
	}
	std::string strName(Name.s, Name.size);
	for (auto it : candidates)
	{
		Scope* s = dynamic_cast<Scope*>(it);
		if (s)
		{
			int idx = s->AddOrGet(strName,
				!m_isLeftValue);
			if (idx != -1)
			{//use the scope to find this name as its scope
				m_scope = s;
				Index = idx;
				break;
			}
		}
	}
}

void Var::ScopeLayout()
{
	Scope* pMyScope = GetScope();
	int idx = -1;
	bool bIsLeftValue = m_isLeftValue;
	while (pMyScope != nullptr && idx <0)
	{
		std::string strName(Name.s, Name.size);
		idx = pMyScope->AddOrGet(strName,
			!m_isLeftValue);
		if (idx == (int)ScopeVarIndex::EXTERN)
		{//for extern var, always looking up parent scopes
			bIsLeftValue = false;
		}
		if (bIsLeftValue)
		{//Left value will add into local scope
		//don't need to go up
			break;
		}
		if (idx >=0)
		{//use the scope to find this name as its scope
			m_scope = pMyScope;
			break;
		}
		pMyScope = pMyScope->GetParentScope();
	}
	Index = idx;
}
}
}