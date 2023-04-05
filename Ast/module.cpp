#include "module.h"
#include "builtin.h"
#include "object.h"
#include "function.h"
#include "event.h"
#include <iostream>
#include "PyEngObject.h"
#include "dbg.h"
#include "port.h"
#include "moduleobject.h"

namespace X 
{
namespace AST 
{
void Module::SetDebug(bool b,XlangRuntime* runtime)
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
	std::string self("self");
	AddOrGet(self,false);
	auto& funcs = Builtin::I().All();
	for (auto it : funcs)
	{
		int idx = AddOrGet(it.name, false);
	}
	Builtin::I().ReturnMap();
	Block::ScopeLayout();
}
void Module::AddBuiltins(XlangRuntime* rt)
{
	auto& funcs = Builtin::I().All();
	m_stackFrame->SetVarCount(GetVarNum());
	//add self as this module
	{
		std::string selfName("self");
		int idx = AddOrGet(selfName, true);
		if (idx >= 0)
		{
			auto* pModuleObj = new ModuleObject(this);
			Value v0(pModuleObj);
			Set(rt, nullptr, idx, v0);
		}
	}
	for (auto it : funcs)
	{
		int idx = AddOrGet(it.name, true);
		if (idx >= 0)
		{
			Value v0(it.funcObj);
			Set(rt,nullptr,idx, v0);
		}
	}
	Builtin::I().ReturnMap();
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
		X::Value valTid(hitSessionTid);
		kwParams.Add("tid", valTid);
		X::Value valAction("notify");
		kwParams.Add("action", valAction);
		const int online_len = 1000;
		char strBuf[online_len];
		SPRINTF(strBuf, online_len, "[{\"HitBreakpoint\":%d}]", line);
		X::Value valParam(strBuf);
		kwParams.Add("param", valParam);
		std::cout << "HitBreakpoint in line:" << line << std::endl;
		std::string evtName("devops.dbg");
		ARGS params(0);
		X::EventSystem::I().Fire(nullptr,nullptr,evtName,params,kwParams);
	}
	return bHit;
}
}
}