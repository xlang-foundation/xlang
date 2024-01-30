#pragma once

#include "scope.h"
#include "stackframe.h"
#include "function.h"

namespace X
{
	template<int N>
	class Obj_Func_Scope
	{
		AST::StackFrame* m_variableFrame = nullptr;
		AST::Scope* m_pMyScope = nullptr;
	public:
		Obj_Func_Scope()
		{
		}
		FORCE_INLINE AST::Scope* GetMyScope()
		{
			return m_pMyScope;
		}
		void Clean()
		{
			if (m_variableFrame)
			{
				delete m_variableFrame;
				m_variableFrame = nullptr;
			}
			if (m_pMyScope)
			{
				delete m_pMyScope;
				m_pMyScope = nullptr;
			}
		}
		~Obj_Func_Scope()
		{
			Clean();
		}
		void Init()
		{
			m_pMyScope = new AST::Scope();
			m_variableFrame = new AST::StackFrame();
			m_variableFrame->SetVarCount(N);
			m_pMyScope->SetVarFrame(m_variableFrame);

		}
		void Close()
		{
			m_pMyScope->SetNoAddVar(true);
		}
		void AddConst(const char* name, X::Value& val)
		{
			std::string strName(name);
			SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strName, false);
			m_variableFrame->Set(idx, val);
		}
		void AddFunc(const char* name,const char* doc, U_FUNC func)
		{
			std::string strName(name);
			AST::ExternFunc* extFunc = new AST::ExternFunc(strName, doc, func);
			auto* pFuncObj = new X::Data::Function(extFunc);
			pFuncObj->IncRef();
			SCOPE_FAST_CALL_AddOrGet0(idx,m_pMyScope,strName, false);
			Value funcVal(pFuncObj);
			m_variableFrame->Set(idx, funcVal);
		}
	};
}