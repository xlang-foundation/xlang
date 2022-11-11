#pragma once
#include "stackframe.h"
#include "utility.h"
#include "xlang.h"
#include <unordered_map>
#include <vector>

namespace X {
namespace AST
{
	class Module;
	class Scope;
}
class XlangRuntime;
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
	XlangRuntime* rt,
	XObj* pContext,
	AST::StackFrame* frame,
	TraceEvent traceEvent,
	AST::Scope* pThisBlock,
	AST::Expression* pCurrentObj);

struct WritePadInfo
{
	X::Value obj;//has WritePad Function
	X::Value writePadFunc;
	std::string alias;
};
class XlangRuntime:
	public XRuntime
{
	long long m_threadId = 0;
	AST::Module* m_pModule = nullptr;
	AST::StackFrame* m_stackBottom = nullptr;
	XTraceFunc m_tracefunc = nullptr;

	std::vector<WritePadInfo> m_WritePads;
	std::unordered_map<std::string, int> m_WritePadMap;
public:
	XlangRuntime()
	{
		m_threadId = GetThreadID();
	}
	~XlangRuntime();
	inline void SetTrace(XTraceFunc f)
	{
		m_tracefunc = f;
	}
	bool CallWritePads(Value& input,Value& indexOrAlias);
	int PushWritePad(X::Value valObj, std::string alias);
	inline void PopWritePad()
	{
		int size = (int)m_WritePads.size();
		if (size == 0)
		{
			return;
		}
		auto last = m_WritePads[size - 1];
		if (!last.alias.empty())
		{
			auto it = m_WritePadMap.find(last.alias);
			m_WritePadMap.erase(it);
		}
		m_WritePads.erase(m_WritePads.end()-1);
	}
	virtual bool CreateEmptyModule() override;
	inline XTraceFunc GetTrace() { return m_tracefunc; }
	inline long long GetThreadId() { return m_threadId; }
	inline void MirrorStacksFrom(XlangRuntime* rt)
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
	inline void AdjustStack(int varNum)
	{
		if (m_stackBottom)
		{
			m_stackBottom->SetVarCount(varNum);
		}
	}
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