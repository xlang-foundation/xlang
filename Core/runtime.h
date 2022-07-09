#pragma once
#include "stackframe.h"
#include "utility.h"

namespace X {
namespace AST
{
	class Module;
	class Scope;
}
class Runtime
{
	long long m_threadId = 0;
	AST::Module* m_pModule = nullptr;
	AST::StackFrame* m_stackBottom = nullptr;
public:
	Runtime()
	{
		m_threadId = GetThreadID();
	}
	inline long long GetThreadId() { return m_threadId; }
	inline void MirrorStacksFrom(Runtime* rt)
	{
		m_pModule = rt->m_pModule;
		m_stackBottom = rt->m_stackBottom;
	}
	inline void SetCurrentExpr(AST::Expression* expr)
	{
		if (m_stackBottom)
		{
			m_stackBottom->SetCurrentExpr(expr);
		}
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
	inline virtual bool Set(AST::Scope* s,void* pContext, int idx, AST::Value& v)
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
	inline bool SetReturn(AST::Value& v)
	{
		m_stackBottom->SetReturn(v);
		return true;
	}
	inline virtual bool Get(AST::Scope* s,void* pContext, int idx, 
		AST::Value& v,AST::LValue* lValue = nullptr)
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