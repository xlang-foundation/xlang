/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "stackframe.h"
#include "utility.h"
#include "xlang.h"
#include "objref.h"
#include "Locker.h"
#include "wait.h"
#include <unordered_map>
#include <vector>

namespace X {
namespace AST
{
	class Module;
	class Scope;
	class Expression;
}

enum class dbg
{
	None,
	Continue,
	Step,
	StepIn,
	StepOut,
	StackTrace,
	GetRuntime,
	Terminate,
};

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
class CommandInfo;
typedef void (*CommandProcessProc)(XlangRuntime* rt,
	XObj* pContextCurrent,
	CommandInfo* pCommandInfo,
	X::Value& retVal);
class CommandInfo :
	virtual public ObjRef
{
	Locker _lock;
public:
	FORCE_INLINE virtual int IncRef()
	{
		AutoLock lock(_lock);
		return ObjRef::AddRef();
	}
	FORCE_INLINE virtual int DecRef()
	{
		_lock.Lock();
		int ref = ObjRef::Release();
		if (ref == 0)
		{
			_lock.Unlock();
			delete this;
		}
		else
		{
			_lock.Unlock();
		}
		return ref;
	}
	dbg dbgType;

	void* m_callContext = nullptr;
	//vars below used in BuildLocals and BuildObjectContent
	TraceEvent m_traceEvent = TraceEvent::None;
	AST::StackFrame* m_frameId;
	int m_threadId = 0;
	AST::Expression* m_pExpToRun = nullptr;
	X::Value m_varParam;//for input when add command
	bool m_needRetValue = false;
	std::string m_retValueHolder;//for command output

	CommandProcessProc m_process = nullptr;
	XWait* m_wait = nullptr;
};
class XlangRuntime:
	public XRuntime
{
	long long m_threadId = 0;
	std::string m_name;//top stack name( function name)
	bool m_noThreadBinding = false;
	AST::Module* m_pModule = nullptr;
	//AST::Module* m_pDbgModule = nullptr;
	AST::StackFrame* m_stackBottom = nullptr;
	//when thread is created, this is the mirror stack
	//will set as this stack chain's parent
	AST::StackFrame* m_mirrorStack = nullptr;
	XTraceFunc m_tracefunc = nullptr;

	std::vector<WritePadInfo> m_WritePads;
	std::unordered_map<std::string, int> m_WritePadMap;

	dbg m_dbgLastRequest = dbg::Continue;
	dbg m_dbg = dbg::Continue;

	XWait m_commandWait;
	Locker m_lockCommands;
	std::vector<CommandInfo*> m_commands;
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
	bool m_bNoDbg = false; // do not trace
	bool m_bStoped = false; // stopped on breakpoint or step
	AST::Expression* m_pFirstStepOutExp = nullptr;
	FORCE_INLINE void SetDbgType(dbg d, dbg lastRequest)
	{
		m_dbg = d;
		m_dbgLastRequest = lastRequest;
	}
	FORCE_INLINE dbg GetDbgType() { return m_dbg; }
	FORCE_INLINE dbg GetLastRequestDgbType() { return m_dbgLastRequest; }
	bool GetWritePadNum(int& count, int& dataBindingCount);
	bool CallWritePads(Value& fmtString,Value& bindingString,
		Value& indexOrAlias,
		std::vector<Value> Value_Bind_list);
	int PushWritePad(X::Value valObj, std::string alias);
	void PopWritePad();
	virtual bool CreateEmptyModule() override;
	FORCE_INLINE void SetName(std::string& name) { m_name = name; }
	FORCE_INLINE std::string& GetName() { return m_name; }
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
	//FORCE_INLINE void SetDbgM(AST::Module* m) { m_pDbgModule = m; }
	FORCE_INLINE AST::Module* M() { return m_pModule; }
	//FORCE_INLINE AST::Module* DbgM() { return m_pDbgModule; }
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

	FORCE_INLINE void AddCommand(CommandInfo* pCmdInfo, bool bWaitFinish)
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
	FORCE_INLINE CommandInfo* PopCommand()
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
			m_bStoped = true; //should get runtime by thread id
			bRet = m_commandWait.Wait(-1);
			m_bStoped = false;
			m_lockCommands.Lock();
		}
		if (bRet && m_commands.size() > 0)
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
};
}