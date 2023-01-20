#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "Locker.h"
#include "wait.h"
#include "utility.h"
#include <iostream>
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
typedef void (*CommandProcessProc)(XlangRuntime* rt,
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
	bool m_needRetValue = false;
	std::string m_retValueHolder;//for command output

	CommandProcessProc m_process = nullptr;
	XWait* m_wait = nullptr;
	bool m_downstreamDelete = false;//when the downstream get this command
	//need to delete this pointer
};

class Module :
	virtual public Block,
	virtual public Scope
{
	XlangRuntime* m_pRuntime;//for top module, we need it

	Locker m_lockSearchPath;
	std::vector<std::string> m_searchPaths;
	std::string m_moduleName;
	std::string m_path;
	std::string m_code;
	StackFrame* m_stackFrame = nullptr;
	//Parameters
	std::vector<std::string> m_args;
	//for debug
	//Locker m_addCommandLock;
	bool m_inDebug = false;
	dbg m_dbgLastRequest = dbg::Continue;
	dbg m_dbg = dbg::Continue;
	std::vector<Scope*> m_dbgScopes;
	XWait m_commandWait;
	Locker m_lockCommands;
	std::vector<CommandInfo*> m_commands;
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
	void SetRT(XlangRuntime* rt)
	{
		m_pRuntime = rt;
	}
	XlangRuntime* GetRT()
	{
		return m_pRuntime;
	}
	StackFrame* GetStack()
	{
		return m_stackFrame;
	}
	void SetArgs(std::vector<std::string>& args)
	{
		m_args = args;
	}
	std::vector<std::string>& GetArgs()
	{
		return m_args;
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
	inline void AddCommand(CommandInfo* pCmdInfo,bool bWaitFinish)
	{
		//m_addCommandLock.Lock();
		if (bWaitFinish)
		{
			pCmdInfo->m_wait = new XWait();
		}
		//auto tid = GetThreadID();
		//std::cout << "AddCommand,before add,bWaitFinish=" 
		//	<< bWaitFinish<<"pCmdInfo="<< pCmdInfo <<"tid="<<tid << std::endl;
		m_lockCommands.Lock();
		m_commands.push_back(pCmdInfo);
		m_lockCommands.Unlock();
		m_commandWait.Release();
		//std::cout << "AddCommand,after add,pCmdInfo="<< pCmdInfo << "tid=" << tid << std::endl;
		if (bWaitFinish)
		{
			pCmdInfo->m_wait->Wait(-1);
			delete pCmdInfo->m_wait;
		}
		//std::cout << "AddCommand,end,pCmdInfo = "<< pCmdInfo << "tid=" << tid << std::endl;
		//m_addCommandLock.Unlock();
	}
	inline CommandInfo* PopCommand()
	{
		//auto tid = GetThreadID();
		//std::cout << "PopCommand,Begin"<< "tid=" << tid << std::endl;
		bool bRet = true;
		CommandInfo* pCommandInfo = nullptr;
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
			pCommandInfo = m_commands[0];
			m_commands.erase(m_commands.begin());
		}
		m_lockCommands.Unlock();
		//std::cout << "PopCommand,end,pCommandInfo=" << pCommandInfo << "tid=" << tid << std::endl;
		return pCommandInfo;
	}
	inline char* SetCode(char* code, int size)
	{
		int pos = (int)m_code.size();
		if (pos > 0)
		{
			m_code += "\n";
			pos++;//skip the new added char
		}
		m_code += code;
		char* szCode = (char*)m_code.c_str();
		return  szCode + pos;
	}
	std::string& GetCode() { return m_code; }
	~Module()
	{
		m_lockCommands.Lock();
		for (auto& it : m_commands)
		{
			if (it->m_wait)
			{
				it->m_wait->Release();
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
	void AddBuiltins(XlangRuntime* rt);
	inline void Add(XlangRuntime* rt, std::string& name,
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
	virtual void AddAndSet(XlangRuntime* rt, XObj* pContext, std::string& name, Value& v) override
	{
		Add(rt,name, pContext,v);
	}
	virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}

	void SetDebug(bool b,XlangRuntime* runtime);
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
			last->DecRef();
			s->IncRef();
			m_dbgScopes[m_dbgScopes.size() - 1] = s;
		}
	}
	inline void AddDbgScope(Scope* s)
	{
		s->IncRef();
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
				s0->DecRef();
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