#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "Locker.h"
#include "wait.h"

namespace X
{
namespace AST
{
enum class dbg
{
	Continue,
	Step,
	StepIn,
	StepOut,
	StackTrace,
	GetRuntime,
};
struct BreakPointInfo
{
	int line;
	int sessionTid;
};
struct CommandInfo
{
	dbg dbgType;
	void** m_valPlaceholder = nullptr;
	void** m_valPlaceholder2=nullptr;
	void** m_valPlaceholder3 = nullptr;
	XWait* m_wait = nullptr;
};
class Module :
	public Block,
	public Scope
{
	std::string m_moduleName;
	std::string m_code;
	StackFrame* m_stackFrame = nullptr;

	//for debug
	bool m_inDebug = false;
	dbg m_dbg = dbg::Continue;
	std::vector<Expression*> m_dbgScopes;
	Expression* m_curRunningExpr = nil;
	XWait m_commandWait;
	Locker m_lockCommands;
	std::vector<CommandInfo> m_commands;
	Locker m_lockBreakpoints;
	std::vector<BreakPointInfo> m_breakpoints;

public:
	Module() :
		Scope(),
		Block()
	{
		m_type = ObType::Module;
		m_stackFrame = new StackFrame(this);
		SetIndentCount({ 0,-1,-1 });//then each line will have 0 indent
	}
	void SetModuleName(std::string& name)
	{
		m_moduleName = name;
	}
	int SetBreakpoint(int line,int sessionTid);
	bool HitBreakpoint(int line);
	std::string& GetModuleName()
	{
		return m_moduleName;
	}
	inline void AddCommand(CommandInfo& cmdInfo,bool bWaitFinish)
	{
		if (bWaitFinish)
		{
			cmdInfo.m_wait = new XWait();
		}
		m_lockCommands.Lock();
		m_commands.push_back(cmdInfo);
		m_lockCommands.Unlock();
		m_commandWait.Release();
		if (bWaitFinish)
		{
			cmdInfo.m_wait->Wait(-1);
			delete cmdInfo.m_wait;
		}
	}
	inline bool PopCommand(CommandInfo& cmdInfo)
	{
		bool bRet = true;
		m_lockCommands.Lock();
		if (m_commands.size() == 0)
		{
			bRet = false;
			m_lockCommands.Unlock();
			bRet = m_commandWait.Wait(-1);
			m_lockCommands.Lock();
		}
		if (bRet && m_commands.size()>0)
		{
			cmdInfo = m_commands[0];
			m_commands.erase(m_commands.begin());
		}
		m_lockCommands.Unlock();
		return bRet;
	}
	inline void SetCode(char* code, int size)
	{
		m_code = code;
	}
	std::string& GetCode() { return m_code; }
	~Module()
	{
		m_lockCommands.Lock();
		for (auto& it : m_commands)
		{
			if (it.m_wait)
			{
				it.m_wait->Release();
			}
		}
		m_lockCommands.Unlock();
		m_commandWait.Release();
		delete m_stackFrame;
	}
	virtual Scope* GetParentScope() override
	{
		return nullptr;
	}
	virtual void ScopeLayout() override;
	void AddBuiltins(Runtime* rt);
	inline void SetCurExpr(Expression* pExpr)
	{
		m_curRunningExpr = pExpr;
	}
	inline void Add(Runtime* rt, std::string& name,
		void* pContext, Value& v)
	{
		int idx = AddOrGet(name, false);
		if (idx >= 0)
		{
			//changed
			m_stackFrame->SetVarCount(GetVarNum());
			Set(rt, pContext, idx, v);
		}
	}
	// Inherited via Scope
	virtual bool Set(Runtime* rt, void* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(Runtime* rt, void* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}

	inline void SetDebug(bool b)
	{
		m_inDebug = b;
	}
	inline bool IsInDebug()
	{
		return m_inDebug;
	}
	inline Expression* GetCurExpr()
	{
		return m_curRunningExpr;
	}
	inline void SetDbgType(dbg d)
	{
		m_dbg = d;
	}
	inline dbg GetDbgType() { return m_dbg; }
	inline bool InDbgScope(Expression* s)
	{ 
		Scope* pCheckScope = dynamic_cast<Scope*>(s);
		if (pCheckScope == nullptr)
		{
			pCheckScope = s->GetScope();
			s = dynamic_cast<Expression*>(pCheckScope);
		}
		if (s == this)
		{
			return true;
		}
		bool bIn = false;
		for (auto it : m_dbgScopes)
		{
			if (it == s)
			{
				bIn = true;
				break;
			}
		}
		
		return bIn;
	}
	inline void AddDbgScope(Expression* s)
	{
		m_dbgScopes.push_back(s);
	}
	inline void RemoveDbgScope(Expression* s)
	{
		auto it = m_dbgScopes.begin();
		while (it != m_dbgScopes.end())
		{
			if (*it == s)
			{
				m_dbgScopes.erase(it);
				break;
			}
			else
			{
				++it;
			}
		}
	}
};
}
}