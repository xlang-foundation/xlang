#pragma once
#include "exp.h"
#include "scope.h"
#include "block.h"
#include "Locker.h"
#include "wait.h"
#include "utility.h"
#include <iostream>
#include "value.h"
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

class CommandInfo;
typedef void (*CommandProcessProc)(XlangRuntime* rt,
	XObj* pContextCurrent,
	CommandInfo* pCommandInfo,
	X::Value& retVal);
class CommandInfo:
	virtual public ObjRef
{
	Locker _lock;
public:
	inline virtual int IncRef()
	{
		AutoLock(_lock);
		return ObjRef::AddRef();
	}
	inline virtual int DecRef()
	{
		AutoLock(m_lock);
		return ObjRef::Release();
	}

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
};
enum class module_primitive
{
	Output,
	Count,
};

struct PrimitiveInfo
{
	Value primitive;
	XlangRuntime* rt;
};
class Module :
	virtual public Block,
	virtual public Scope
{
	XlangRuntime* m_pRuntime=nullptr;//for top module, we need it
	PrimitiveInfo m_primitives[(int)module_primitive::Count];
	Locker m_lockSearchPath;
	std::vector<std::string> m_searchPaths;
	std::string m_moduleName;
	std::string m_path;
	// for var name and value with str
	// we keep string pointer not copy from the source code memory
	// when pass entire code once per call, this method works well
	// but we work with code fragments for example: interactive mode
	// or work with iPyKernel mode, code added multiple times
	//and parsed multiple time, so the memory is not linear
	//so we keep code as a list of string
	//and each parse use its own code fragment
	//then this way will work fine for multiple parsing times
	std::vector<std::string> m_allCode;
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
	void SetPrimitive(std::string& name, Value& valObj,XRuntime* rt)
	{
		if (name == "Output")
		{
			m_primitives[(int)module_primitive::Output] = { valObj,
				dynamic_cast<XlangRuntime*>(rt) };
		}
	}
	PrimitiveInfo& GetPrimitive(module_primitive idx)
	{
		return m_primitives[(int)idx];
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
		std::string code = GetCode();
		if (endPos >= code.size())
		{
			return code.substr(startPos);
		}
		else
		{
			return code.substr(startPos, endPos - startPos+1);
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
		pCmdInfo->IncRef();//for keeping in m_commands
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
			// pCommandInfo already have refcount when adding into m_commands
			//so don't call IncRef here
			m_commands.erase(m_commands.begin());
		}
		m_lockCommands.Unlock();
		//std::cout << "PopCommand,end,pCommandInfo=" << pCommandInfo << "tid=" << tid << std::endl;
		return pCommandInfo;
	}
	inline char* SetCode(char* code, int size)
	{
		std::string strCode(code, size);
		m_allCode.push_back(strCode);
		char* szCode = (char*)m_allCode[m_allCode.size()-1].c_str();
		return  szCode;
	}
	//combine all code fragments and insert newline between fragments
	std::string GetCode() 
	{ 
		if (m_allCode.size() == 0)
		{
			return "";
		}
		std::string code = m_allCode[0];
		for (int i=1;i<(int)m_allCode.size();i++)
		{
			code += "\n"+ m_allCode[i];
		}
		return code; 
	}
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