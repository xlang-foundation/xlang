#include "module.h"
#include "builtin.h"
#include "object.h"
#include "function.h"
#include "event.h"
#include <iostream>
#include "PyEngObject.h"
#include "dbg.h"
#include "port.h"

namespace X 
{
namespace AST 
{
void Module::SetDebug(bool b,Runtime* runtime)
{
	m_inDebug = b;
	if (b)
	{
		runtime->SetTrace(Dbg::xTraceFunc);
		PyEng::Object objRT((unsigned long long)runtime);
		PyEng::Object::SetTrace(Dbg::PythonTraceFunc,objRT);
	}
	else
	{
		runtime->SetTrace(nullptr);
		PyEng::Object::SetTrace(nullptr, nullptr);
	}
}
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
void Module::ClearBreakpoints()
{
	m_lockBreakpoints.Lock();
	m_breakpoints.clear();
	m_lockBreakpoints.Unlock();
}
//return the actual line
//-1 means no actual line matched with input line
int Module::SetBreakpoint(int line, int sessionTid)
{
	m_lockBreakpoints.Lock();
	m_breakpoints.push_back({ line,sessionTid });
	m_lockBreakpoints.Unlock();
	return line;
}
bool Module::HitBreakpoint(int line)
{
	bool bHit = false;
	int hitSessionTid = 0;
	m_lockBreakpoints.Lock();
	for (auto it : m_breakpoints)
	{
		if (it.line == line)
		{
			bHit = true;
			hitSessionTid = it.sessionTid;
			break;
		}
	}
	m_lockBreakpoints.Unlock();
	if (bHit)
	{
		KWARGS kwParams;
		kwParams.emplace(std::make_pair("tid", hitSessionTid));
		kwParams.emplace(std::make_pair("action","notify"));
		const int online_len = 1000;
		char strBuf[online_len];
		SPRINTF(strBuf, online_len, "[{\"HitBreakpoint\":%d}]", line);
		kwParams.emplace(std::make_pair("param", strBuf));
		std::cout << "HitBreakpoint in line:" << line << std::endl;
		std::string evtName("devops.dbg");
		ARGS params;
		X::EventSystem::I().Fire(nullptr,nullptr,evtName,params,kwParams);
	}
	return bHit;
}
}
}