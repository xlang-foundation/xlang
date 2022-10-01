#pragma once
#include "stackframe.h"
#include "utility.h"
#include "xlang.h"

namespace X {
namespace AST
{
	class Module;
	class Scope;
}
class Runtime;
enum class TraceEvent
{
	None=-1,
	Call = 0,
	Exception = 1,
	Line = 2,
	Return = 3,
	C_Call = 4,
	C_Exception = 5,
	C_Return = 6,
	OPCode = 7,
	HitBreakpoint =10,
};
typedef bool (*XTraceFunc)(
	Runtime* rt,
	XObj* pContext,
	AST::StackFrame* frame,
	TraceEvent traceEvent,
	AST::Scope* pThisBlock,
	AST::Expression* pCurrentObj);

class Runtime:
	public XRuntime
{
	long long m_threadId = 0;
	AST::Module* m_pModule = nullptr;
	AST::StackFrame* m_stackBottom = nullptr;
	XTraceFunc m_tracefunc = nullptr;
public:
	Runtime()
	{
		m_threadId = GetThreadID();
	}
	inline void SetTrace(XTraceFunc f)
	{
		m_tracefunc = f;
	}
	virtual bool CreateEmptyModule() override;
	inline XTraceFunc GetTrace() { return m_tracefunc; }
	inline long long GetThreadId() { return m_threadId; }
	inline void MirrorStacksFrom(Runtime* rt)
	{
		m_pModule = rt->m_pModule;
		m_stackBottom = rt->m_stackBottom;
	}
	inline bool SetVarCount(int cnt)
	{
		return m_stackBottom ? m_stackBottom->SetVarCount(cnt) : false;
	}
	inline void SetM(AST::Module* m) { m_pModule = m; }
	inline AST::Module* M() { return m_pModule; }
	inline void PushFrame(AST::StackFrame* frame,int varNum)
	{
		frame->SetVarCount(varNum);
		if (m_stackBottom)
		{
			m_stackBottom->SetNext(frame);
		}
		m_stackBottom = frame;
	}
	inline void PopFrame()
	{
		if (m_stackBottom!=nullptr)
		{
			m_stackBottom = m_stackBottom->Prev();
			if(m_stackBottom) m_stackBottom->SetNext(nullptr);
		}
	}
	inline AST::StackFrame* GetCurrentStack()
	{
		return m_stackBottom;
	}
	inline virtual bool DynSet(AST::Scope* s, XObj* pContext, int idx, X::Value& v)
	{
		int cnt = m_stackBottom->GetVarCount();
		if (cnt <= idx)
		{
			m_stackBottom->SetVarCount(idx + 1);
		}
		return Set(s, pContext, idx, v);
	}
	inline virtual bool Set(AST::Scope* s,XObj* pContext, int idx, X::Value& v)
	{
		bool bOK = false;
		auto it = m_stackBottom;
		while (it!= nullptr)
		{
			if (it->belongTo(s))
			{
				it->Set(idx, v);
				bOK = true;
				break;
			}
			it = it->Prev();
		}
		return bOK;
	}
	inline bool SetReturn(X::Value& v)
	{
		m_stackBottom->SetReturn(v);
		return true;
	}
	inline virtual bool Get(AST::Scope* s,XObj* pContext, int idx, 
		X::Value& v,X::LValue* lValue = nullptr)
	{
		bool bOK = false;
		auto it = m_stackBottom;
		while (it != nullptr)
		{
			if (it->belongTo(s))
			{
				it->Get(idx, v, lValue);
				bOK = true;
				break;
			}
			it = it->Prev();
		}
		return bOK;
	}
};
}