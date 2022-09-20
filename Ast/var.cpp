#include "var.h"
#include "value.h"
#include "object.h"
#include "prop.h"
#include <iostream>

namespace X
{
namespace AST
{
	bool Var::ToBytes(Runtime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Expression::ToBytes(rt, pContext, stream);
		stream << Name.size;
		if (Name.size > 0)
		{
			stream.append(Name.s, Name.size);
		}
		stream << Index;
		//check the value if it is external
		if (m_scope != stream.ScopeSpace().GetCurrentScope())
		{
			std::cout << "Var::ToBytes" << std::endl;
		}
		return true;
	}
	bool Var::GetPropValue(Runtime* rt, XObj* pContext,XObj* pObj, Value& val)
	{
		bool bOK = false;
		auto* pPropObj = dynamic_cast<Data::PropObject*>(pObj);
		if (pPropObj)
		{
			bOK = pPropObj->GetProp(rt, pContext, val);
		}
		return bOK;
	}
bool Var::CalcCallables(Runtime* rt, XObj* pContext,
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