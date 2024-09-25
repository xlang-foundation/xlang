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
		FORCE_INLINE void InitWithNumber(int cnt)
		{
			m_pMyScope = new AST::Scope();
			m_variableFrame = new AST::StackFrame();
			m_variableFrame->SetVarCount(cnt);
			m_pMyScope->SetVarFrame(m_variableFrame);

		}
		FORCE_INLINE void Init()
		{
			InitWithNumber(N);
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
		void AddObject(const char* name, X::Value& object)
		{
			std::string strName(name);
			SCOPE_FAST_CALL_AddOrGet0(idx, m_pMyScope, strName, false);
			m_variableFrame->Set(idx, object);
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