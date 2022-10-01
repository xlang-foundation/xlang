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

struct CommandInfo;
typedef void (*CommandProcessProc)(Runtime* rt,
	XObj* pContextCurrent,
	CommandInfo* pCommandInfo,
	X::Value& retVal);
struct CommandInfo
{
	dbg dbgType;

	void* m_callContext = nullptr;
	//vars below used in BuildLocals and BuildObjectContent
	TraceEvent m_traceEvent= TraceEvent::None;
	int m_frameId;
	AST::Expression* m_pExpToRun = nullptr;
	X::Value m_varParam;//for input when add command
	X::Value* m_retValueHolder = nullptr;//for command output

	CommandProcessProc m_process = nullptr;
	XWait* m_wait = nullptr;
};

class Module :
	virtual public Block,
	virtual public Scope
{
	Locker m_lockSearchPath;
	std::vector<std::string> m_searchPaths;
	std::string m_moduleName;
	std::string m_path;
	std::string m_code;
	StackFrame* m_stackFrame = nullptr;

	//for debug
	bool m_inDebug = false;
	dbg m_dbgLastRequest = dbg::Continue;
	dbg m_dbg = dbg::Continue;
	std::vector<Scope*> m_dbgScopes;
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
	void GetSearchPaths(std::vector<std::string>& searchPaths)
	{
		m_lockSearchPath.Lock();
		searchPaths = m_searchPaths;
		m_lockSearchPath.Unlock();
	}
	std::string GetCodeFragment(int startPos, int endPos)
	{
		if (endPos >= m_code.size())
		{
			return m_code.substr(startPos);
		}
		else
		{
			return m_code.substr(startPos, endPos - startPos+1);
		}
	}
	void AddSearchPath(std::string& strPath)
	{
		RemoveSearchPath(strPath);//remove duplicated
		m_lockSearchPath.Lock();
		m_searchPaths.push_back(strPath);
		m_lockSearchPath.Unlock();
	}
	void RemoveSearchPath(std::string& strPath)
	{
		m_lockSearchPath.Lock();
		for (auto it = m_searchPaths.begin();it != m_searchPaths.end();)
		{
			if (*it == strPath)
			{
				it = m_searchPaths.erase(it);
			}
			else
			{
				++it;
			}
		}
		m_lockSearchPath.Unlock();
	}
	void SetModuleName(std::string& name)
	{
		auto pos = name.rfind("/");
		if (pos == name.npos)
		{
			pos = name.rfind("\\");
		}
		if (pos != name.npos)
		{
			m_path = name.substr(0, pos);
		}
		m_moduleName = name;
	}
	void ClearBreakpoints();
	int SetBreakpoint(int line,int sessionTid);
	bool HitBreakpoint(int line);
	std::string& GetModulePath()
	{
		return m_path;
	}
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
	inline void Add(Runtime* rt, std::string& name,
		XObj* pContext, Value& v)
	{
		int idx = AddOrGet(name, false);
		if (idx >= 0)
		{
			int cnt = m_stackFrame->GetVarCount();
			if (cnt <= idx)
			{
				m_stackFrame->SetVarCount(idx + 1);
			}
			Set(rt, pContext, idx, v);
		}
	}
	// Inherited via Scope
	virtual void AddAndSet(Runtime* rt, XObj* pContext, std::string& name, Value& v) override
	{
		Add(rt,name, pContext,v);
	}
	virtual bool Set(Runtime* rt, XObj* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(Runtime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}

	void SetDebug(bool b,Runtime* runtime);
	inline bool IsInDebug()
	{
		return m_inDebug;
	}
	inline void SetDbgType(dbg d,dbg lastRequest)
	{
		m_dbg = d;
		m_dbgLastRequest = lastRequest;
	}
	inline dbg GetDbgType() { return m_dbg; }
	inline dbg GetLastRequestDgbType() { return m_dbgLastRequest; }
	inline bool InDbgScope(Scope* s)
	{ 
		if (s == this)
		{
			return true;
		}
		bool bIn = false;
		if (m_dbgScopes.size() > 0)
		{
			Scope* last = m_dbgScopes[m_dbgScopes.size() - 1];
			if (last->isEqual(s))
			{
				bIn = true;
			}
		}	
		return bIn;
	}
	inline Scope* LastScope()
	{
		return m_dbgScopes.size() > 0 ? 
			m_dbgScopes[m_dbgScopes.size() - 1] : nullptr;
	}
	inline ScopeWaitingStatus HaveWaitForScope()
	{
		return m_dbgScopes.size() > 0?
			m_dbgScopes[m_dbgScopes.size() - 1]->IsWaitForCall():
			ScopeWaitingStatus::NoWaiting;
	}
	inline void ReplaceLastDbgScope(Scope* s)
	{
		if (m_dbgScopes.size() > 0)
		{
			Scope* last = m_dbgScopes[m_dbgScopes.size() - 1];
			last->Release();
			s->AddRef();
			m_dbgScopes[m_dbgScopes.size() - 1] = s;
		}
	}
	inline void AddDbgScope(Scope* s)
	{
		s->AddRef();
		m_dbgScopes.push_back(s);
	}
	inline void RemoveDbgScope(Scope* s)
	{
		auto it = m_dbgScopes.begin();
		while (it != m_dbgScopes.end())
		{
			Scope* s0 = (*it);
			if (s0->isEqual(s))
			{
				s0->Release();
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