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
	bool UsingDataBinding = false;
	std::string alias;
};
class XlangRuntime:
	public XRuntime
{
	long long m_threadId = 0;
	std::string m_name;//top stack name( function name)
	bool m_noThreadBinding = false;
	AST::Module* m_pModule = nullptr;
	AST::StackFrame* m_stackBottom = nullptr;
	//when thread is created, this is the mirror stack
	//will set as this stack chain's parent
	AST::StackFrame* m_mirrorStack = nullptr;
	XTraceFunc m_tracefunc = nullptr;

	std::vector<WritePadInfo> m_WritePads;
	std::unordered_map<std::string, int> m_WritePadMap;
public:
	XlangRuntime()
	{
		m_threadId = GetThreadID();
	}
	void SetNoThreadBinding(bool b)
	{
		m_noThreadBinding = b;
	}
	~XlangRuntime();
	FORCE_INLINE void SetTrace(XTraceFunc f)
	{
		m_tracefunc = f;
	}
	bool GetWritePadNum(int& count, int& dataBindingCount);
	bool CallWritePads(Value& fmtString,Value& bindingString,
		Value& indexOrAlias,
		std::vector<Value> Value_Bind_list);
	int PushWritePad(X::Value valObj, std::string alias);
	void PopWritePad();
	virtual bool CreateEmptyModule() override;
	FORCE_INLINE void SetName(std::string& name) { m_name = name; }
	FORCE_INLINE std::string& GetName() { return m_name; }
	FORCE_INLINE XTraceFunc GetTrace() { return m_tracefunc; }
	FORCE_INLINE long long GetThreadId() { return m_threadId; }
	FORCE_INLINE void MirrorStacksFrom(XlangRuntime* rt)
	{
		m_pModule = rt->m_pModule;
		m_mirrorStack = rt->m_stackBottom;
		m_tracefunc = rt->m_tracefunc;
		//TODO:when stack remove from link, need to check if need to set back
		if (m_mirrorStack)
		{
			m_mirrorStack->SetShareFlag(true);
		}
	}
	FORCE_INLINE bool SetVarCount(int cnt)
	{
		return m_stackBottom ? m_stackBottom->SetVarCount(cnt) : false;
	}
	FORCE_INLINE virtual bool AddVar(const char* name, X::Value& val) override
	{
		std::string strName(name);
		return m_stackBottom ? m_stackBottom->AddVar(this, strName, val) : false;
	}
	FORCE_INLINE void SetM(AST::Module* m) { m_pModule = m; }
	FORCE_INLINE AST::Module* M() { return m_pModule; }
	FORCE_INLINE void AdjustStack(int varNum)
	{
		if (m_stackBottom)
		{
			m_stackBottom->SetVarCount(varNum);
		}
	}
	FORCE_INLINE void PushFrame(AST::StackFrame* frame,int varNum)
	{
		frame->SetVarCount(varNum);
		if (m_stackBottom)
		{
			m_stackBottom->SetNext(frame);
		}
		else
		{
			//set as top frame for this thread
			//and set its parent to mirror stack
			frame->SetParent(m_mirrorStack);
		}
		m_stackBottom = frame;
	}
	FORCE_INLINE void PopFrame()
	{
		if (m_stackBottom!=nullptr)
		{
			m_stackBottom = m_stackBottom->Prev();
			if(m_stackBottom) m_stackBottom->SetNext(nullptr);
		}
	}
	FORCE_INLINE AST::StackFrame* GetCurrentStack()
	{
		return m_stackBottom;
	}
	FORCE_INLINE virtual bool DynSet(AST::Scope* s, XObj* pContext, int idx, X::Value& v)
	{
		int cnt = m_stackBottom->GetVarCount();
		if (cnt <= idx)
		{
			m_stackBottom->SetVarCount(idx + 1);
		}
		return Set(s, pContext, idx, v);
	}
	FORCE_INLINE virtual bool Set(AST::Scope* s,XObj* pContext, int idx, X::Value& v) final
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
	FORCE_INLINE bool SetReturn(X::Value& v)
	{
		m_stackBottom->SetReturn(v);
		return true;
	}
	FORCE_INLINE virtual bool Get(AST::Scope* s,XObj* pContext, int idx, 
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
	virtual X::Value GetXModuleFileName() override;
	virtual int GetTopStackCurrentLine() override;
};
}