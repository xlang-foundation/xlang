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
	namespace Jit
	{
		class JitLib;
	}
namespace AST
{

struct BreakPointInfo
{
	int line;
	int sessionTid; // needed ?
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
	public Block
{

	//if this module has Jit Blocks, will have this member
	X::Jit::JitLib* m_pJitLib = nullptr;

	StackFrame* m_stackFrame = nullptr;
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
	//Parameters
	std::vector<X::Value> m_args;
	//for debug
	//Locker m_addCommandLock;
	
	std::vector<Scope*> m_dbgScopes;
	
	Locker m_lockBreakpoints;
	std::vector<BreakPointInfo> m_breakpoints;
	//only keep for AST Query
	std::vector<X::AST::InlineComment*> m_inlineComments;

	//for scope
	bool m_bMyScopeIsRef = false;
public:
	Module():
		Block()
	{
		m_type = ObType::Module;
		m_pMyScope = new Scope();
		m_pMyScope->SetType(ScopeType::Module);
		m_pMyScope->SetExp(this);
		m_stackFrame = new StackFrame(m_pMyScope);
		m_pMyScope->SetVarFrame(m_stackFrame);
		SetIndentCount({ 0,-1,-1 });//then each line will have 0 indent
	}
	FORCE_INLINE void SetJitLib(X::Jit::JitLib* pLib)
	{
		m_pJitLib = pLib;
	}
	void ChangeMyScopeTo(Scope* pNewMyScope)
	{
		if (!m_bMyScopeIsRef)
		{
			delete m_pMyScope;
		}
		m_pMyScope = pNewMyScope;
		m_bMyScopeIsRef = true;
		m_stackFrame->SetScope(pNewMyScope);
	}
	~Module()
	{
		
		delete m_stackFrame;
		if (!m_bMyScopeIsRef)
		{
			delete m_pMyScope;
		}

		for (auto it : m_inlineComments)
		{
			delete it;
		}
		m_inlineComments.clear();
	}
	FORCE_INLINE Scope* GetMyScope()
	{
		return m_pMyScope;
	}
	void AddInlineComment(X::AST::InlineComment* pExp)
	{
		m_inlineComments.push_back(pExp);
	}
	std::vector<X::AST::InlineComment*>& GetInlineComments()
	{
		return m_inlineComments;
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
	void SetArgs(std::vector<X::Value>& args)
	{
		m_args = args;
	}
	std::vector<X::Value>& GetArgs()
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
	bool HitBreakpoint(XlangRuntime* rt,int line);
	void StopOn(const char* stopType);
	std::string& GetModulePath()
	{
		return m_path;
	}
	std::string& GetModuleName()
	{
		return m_moduleName;
	}
	
	FORCE_INLINE char* SetCode(char* code, int size)
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
	virtual void ScopeLayout() override;
	void AddBuiltins(XlangRuntime* rt);
	FORCE_INLINE int Add(XlangRuntime* rt, std::string& name,
		XObj* pContext, Value& v)
	{
		SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,name, false);
		if (idx >= 0)
		{
			int cnt = m_stackFrame->GetVarCount();
			if (cnt <= idx)
			{
				m_stackFrame->SetVarCount(idx + 1);
			}
			rt->Set(m_pMyScope, pContext, idx, v);
		}
		return idx;
	}
	void SetDebug(bool b,XlangRuntime* runtime);
	FORCE_INLINE bool InDbgScope(Scope* s)
	{ 
		if (s == m_pMyScope)
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
	FORCE_INLINE Scope* LastScope()
	{
		return m_dbgScopes.size() > 0 ? 
			m_dbgScopes[m_dbgScopes.size() - 1] : nullptr;
	}
	FORCE_INLINE ScopeWaitingStatus HaveWaitForScope()
	{
		return m_dbgScopes.size() > 0?
			m_dbgScopes[m_dbgScopes.size() - 1]->IsWaitForCall():
			ScopeWaitingStatus::NoWaiting;
	}
	FORCE_INLINE void ReplaceLastDbgScope(Scope* s)
	{
		if (m_dbgScopes.size() > 0)
		{
			Scope* last = m_dbgScopes[m_dbgScopes.size() - 1];
			m_dbgScopes[m_dbgScopes.size() - 1] = s;
		}
	}
	FORCE_INLINE void AddDbgScope(Scope* s)
	{
		m_dbgScopes.push_back(s);
	}
	FORCE_INLINE void RemoveDbgScope(Scope* s)
	{
		auto rit = m_dbgScopes.rbegin();
		while (rit != m_dbgScopes.rend())
		{
			Scope* s0 = (*rit);
			if (s0->isEqual(s))
			{
				m_dbgScopes.erase((++rit).base());
				break;
			}
			else
			{
				++rit;
			}
		}
	}
};
}
}