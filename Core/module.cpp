#include "module.h"
#include "builtin.h"
#include "object.h"

namespace X 
{
namespace AST 
{
void Module::ScopeLayout()
{
	auto& funcs = Builtin::I().All();
	for (auto it : funcs)
	{
		auto name = it.first;
		int idx = AddOrGet(name, false);
	}
	Block::ScopeLayout();
}
void Module::AddBuiltins(Runtime* rt)
{
	auto& funcs = Builtin::I().All();
	m_stackFrame->SetVarCount(GetVarNum());
	for (auto it : funcs)
	{
		auto name = it.first;
		int idx = AddOrGet(name, true);
		if (idx >= 0)
		{
			auto* pFuncObj = new Data::Function(it.second);
			Value v0(pFuncObj);
			Set(rt,nullptr,idx, v0);
		}
	}
}
}
}