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
#include "dyn_scope.h"

#define SCOPE_FAST_CALL_AddOrGet0_NoRet(myScope,name,bGetOnly) \
	SCOPE_FAST_CALL_AddOrGet_NoRet(myScope,name,bGetOnly,nullptr)

#define SCOPE_FAST_CALL_AddOrGet0(idx,myScope,name,bGetOnly) \
	SCOPE_FAST_CALL_AddOrGet(idx,myScope,name,bGetOnly,nullptr)

#define SCOPE_FAST_CALL_AddOrGet0_NoDef(idx,myScope,name,bGetOnly) \
	SCOPE_FAST_CALL_AddOrGet_NoDef(idx,myScope,name,bGetOnly,nullptr)

#define SCOPE_FAST_CALL_AddOrGet(idx,myScope,name,bGetOnly,ppRightScope) \
	int idx;\
	SCOPE_FAST_CALL_AddOrGet_NoDef(idx,myScope,name,bGetOnly,ppRightScope)

#define SCOPE_FAST_CALL_AddOrGet_NoDef(idx,myScope,name,bGetOnly,ppRightScope)\
	if (myScope->GetNamespaceScope())\
	{\
		idx = myScope->GetNamespaceScope()->AddOrGet(name, bGetOnly, ppRightScope);\
	}\
	else if (myScope->GetDynScope())\
	{\
		idx = myScope->GetDynScope()->AddOrGet(name.c_str(), bGetOnly);\
	}\
	else\
	{\
		idx = myScope->AddOrGet(name, bGetOnly, ppRightScope);\
	}

#define SCOPE_FAST_CALL_AddOrGet_NoRet(myScope,name,bGetOnly,ppRightScope)\
	if (myScope->GetNamespaceScope())\
	{\
		myScope->GetNamespaceScope()->AddOrGet(name, bGetOnly, ppRightScope);\
	}\
	else if (myScope->GetDynScope())\
	{\
		myScope->GetDynScope()->AddOrGet(name.c_str(), bGetOnly);\
	}\
	else\
	{\
		myScope->AddOrGet(name, bGetOnly, ppRightScope);\
	}
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
	DeferredObject,
	RemoteObject,
	Namespace,
	Custom,//impl. XCustomScope in the Object
};
//Variables scope support, for Module and Func/Class

class Expresion;
class Scope
{
	Locker m_lock;
	ScopeType m_type = ScopeType::Module;
	Expression* m_pExp = nullptr;//expression owns this scope for example moudle or func
	DynamicScope* m_pDynScope = nullptr; //to hold dynamic variables
	//used in PacakgeProxy which as Package's instance to share 
	//same namespace scope with Package, but different m_varFrame
	//we add this method to make scope's variable access is very fast
	//by removing all function calls

	Scope* m_pNamespaceScope = nullptr;
	bool m_NoAddVar = false;//if set to true, can't add new var
protected:
	//only used in Class and Core Object to hold member variables or APIs
	StackFrame* m_varFrame = nullptr;
	std::unordered_map <std::string, int> m_Vars;
	std::unordered_map <std::string, AST::Var*> m_ExternVarMap;
public:
	Scope()
	{
	}
	void SetNamespaceScope(Scope* pScope)
	{
		m_pNamespaceScope = pScope;
	}
	FORCE_INLINE Scope* GetNamespaceScope()
	{
		return m_pNamespaceScope;
	}
	FORCE_INLINE void SetVarFrame(StackFrame* pFrame)
	{
		m_varFrame = pFrame;
	}
	FORCE_INLINE void SetDynScope(DynamicScope* pScope)
	{
		m_pDynScope = pScope;
	}
	FORCE_INLINE DynamicScope* GetDynScope()
	{
		return m_pDynScope;
	}
	FORCE_INLINE void SetNoAddVar(bool bNoAdd)
	{
		m_NoAddVar = bNoAdd;
	}
	//use address as ID, just used Serialization
	FORCE_INLINE ExpId ID() { return (ExpId)this; }
	void AddExternVar(AST::Var* var);
	FORCE_INLINE void SetExp(Expression* pExp)
	{
		m_pExp = pExp;
	}
	FORCE_INLINE Expression* GetExp()
	{
		return m_pExp;
	}
	FORCE_INLINE ScopeType GetType() { return m_type; }
	FORCE_INLINE void SetType(ScopeType type) { m_type = type; }

	bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream);
	bool FromBytes(X::XLangStream& stream);
	FORCE_INLINE int GetVarNum()
	{
		return (int)m_Vars.size();
	}
	FORCE_INLINE std::unordered_map <std::string, int>& GetVarMap() 
	{ 
		return m_Vars; 
	}
	FORCE_INLINE std::vector<std::string> GetVarNames()
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
	FORCE_INLINE bool isEqual(Scope* s) { return (this == s); };
	virtual ScopeWaitingStatus IsWaitForCall() 
	{ 
		return ScopeWaitingStatus::NoWaiting;
	};


	FORCE_INLINE int AddOrGet(std::string& name, bool bGetOnly, Scope** ppRightScope=nullptr)
	{
		if (m_pDynScope)
		{
			return m_pDynScope->AddOrGet(name.c_str(), bGetOnly);
		}
		if (m_NoAddVar)
		{
			bGetOnly = true;
		}
		//Always append,no remove, so new item's index is size of m_Vars;
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
	FORCE_INLINE void Set(XlangRuntime* rt, XObj* pContext,int idx, X::Value& v)
	{
		//TODO: check performance here
		if (m_pDynScope)
		{
			m_pDynScope->Set(idx,v);
		}
		else if (m_varFrame)
		{
			m_varFrame->Set(idx, v);
		}
		else
		{
			rt->Set(this, pContext, idx, v);
		}
	}

	FORCE_INLINE void Get(XlangRuntime* rt, XObj* pContext,int idx, X::Value& v, LValue* lValue = nullptr)
	{
		//TODO: check performance here
		if (m_pDynScope)
		{
			m_pDynScope->Get(idx, v, lValue);
		}
		else if (m_varFrame)
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
