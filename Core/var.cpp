#include "var.h"
#include "value.h"
#include "object.h"

namespace X
{
namespace AST
{
bool Var::CalcCallables(Runtime* rt, void* pContext,
		std::vector<Expression*>& callables)
	{
		Value val;
		bool bOK = Run(rt, pContext, val);
		if (bOK && val.IsObject())
		{
			bOK = val.GetObj()->CalcCallables(rt, pContext, callables);
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
	while (pMyScope != nullptr && idx == -1)
	{
		std::string strName(Name.s, Name.size);
		idx = pMyScope->AddOrGet(strName,
			!m_isLeftValue);
		if (m_isLeftValue)
		{//Left value will add into local scope
		//don't need to go up
			break;
		}
		if (idx != -1)
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