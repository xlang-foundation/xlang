#pragma once

#include <string>
#include <unordered_map>
#include "value.h"
#include "runtime.h"
#include <assert.h>
#include <functional>
#include "objref.h"
#include "def.h"
#include "XLangStream.h"
#include "Locker.h"
#include "stackframe.h"

namespace X 
{ 
namespace AST 
{
class Var;
enum class ScopeWaitingStatus
{
	NoWaiting,
	HasWaiting,
	NeedFurtherCallWithName
};
enum class ScopeVarIndex
{
	INVALID =-1,
	EXTERN =-2
};
enum class ScopeType
{
	Module,
	Class,
	Func,
	Package,
	PyObject,
	Namespace,
};
//Variables scope support, for Module and Func/Class

class Expresion;
class Scope
{
	Locker m_lock;
	ScopeType m_type = ScopeType::Module;
	Expression* m_pExp = nullptr;//expression owns this scope for example moudle or func
protected:
	//only used in Class and Core Object to hold member variables or APIs
	StackFrame* m_varFrame = nullptr;
	std::unordered_map <std::string, int> m_Vars;
	std::unordered_map <std::string, AST::Var*> m_ExternVarMap;
public:
	Scope()
	{
	}
	inline void SetVarFrame(StackFrame* pFrame)
	{
		m_varFrame = pFrame;
	}
	//use address as ID, just used Serialization
	ExpId ID() { return (ExpId)this; }
	void AddExternVar(AST::Var* var);
	inline void SetExp(Expression* pExp)
	{
		m_pExp = pExp;
	}
	inline Expression* GetExp()
	{
		return m_pExp;
	}
	inline ScopeType GetType() { return m_type; }
	inline void SetType(ScopeType type) { m_type = type; }

	bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream);
	bool FromBytes(X::XLangStream& stream);
	inline int GetVarNum()
	{
		return (int)m_Vars.size();
	}
	inline std::unordered_map <std::string, int>& GetVarMap() 
	{ 
		return m_Vars; 
	}
	inline std::vector<std::string> GetVarNames()
	{
		std::vector<std::string> names;
		for (auto& it : m_Vars)
		{
			names.push_back(it.first);
		}
		return names;
	}
	void EachVar(XlangRuntime* rt,XObj* pContext,
		std::function<void(std::string,X::Value&)> const& f)
	{
		for (auto it : m_Vars)
		{
			X::Value val;
			Get(rt, pContext,it.second, val);
			f(it.first, val);
		}
	}
	bool isEqual(Scope* s) { return (this == s); };
	virtual ScopeWaitingStatus IsWaitForCall() 
	{ 
		return ScopeWaitingStatus::NoWaiting;
	};

	inline int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope=nullptr)
	{//Always append,no remove, so new item's index is size of m_Vars;
		//check extern map first,if it is extern var
		//just return -1 to make caller look up to parent scopes
		if (m_ExternVarMap.find(name)!= m_ExternVarMap.end())
		{
			return (int)ScopeVarIndex::EXTERN;
		}
		auto it = m_Vars.find(name);
		if (it != m_Vars.end())
		{
			return it->second;
		}
		else if (!bGetOnly)
		{
			int idx = (int)m_Vars.size();
			m_Vars.emplace(std::make_pair(name, idx));
			if (m_varFrame)
			{
				m_varFrame->SetVarCount(m_Vars.size());
			}
			return idx;
		}
		else
		{
			return (int)ScopeVarIndex::INVALID;
		}
	}
	inline void Set(XlangRuntime* rt, XObj* pContext,int idx, X::Value& v)
	{
		if (m_varFrame)
		{
			m_varFrame->Set(idx, v);
		}
		else
		{
			rt->Set(this, pContext, idx, v);
		}
	}

	inline void Get(XlangRuntime* rt, XObj* pContext,int idx, X::Value& v, LValue* lValue = nullptr)
	{
		if (m_varFrame)
		{
			m_varFrame->Get(idx, v, lValue);
		}
		else
		{
			rt->Get(this,pContext, idx, v, lValue);
		}
	}
};
}
}
