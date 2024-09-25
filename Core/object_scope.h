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

namespace X
{
	namespace Data 
	{
		template<class class_T, typename F, typename Array, std::size_t... I>
		FORCE_INLINE auto VarCall_impl(class_T* pThis, F f, Array& a, std::index_sequence<I...>)
		{
			return (pThis->*f)(a[I]...);
		}
		template<std::size_t N, class class_T, typename F, typename T, typename Indices = std::make_index_sequence<N>>
		FORCE_INLINE auto VarCall(class_T* pThis, F f, T& a)
		{
			return VarCall_impl(pThis, f, a, Indices{});
		}

		template<class T>
		class ObjectScope
		{
			AST::Scope* m_pMyScope = nullptr;
		protected:
			struct FuncInfo
			{
				std::string name;
				X::U_FUNC func;
			};
			std::vector<FuncInfo> m_funclist;
			AST::StackFrame* m_variableFrame = nullptr;
			std::vector<X::XEvent*> __events;
		public:
			FORCE_INLINE AST::Scope* GetMyScope()
			{
				return m_pMyScope;
			}
			~ObjectScope()
			{
				if (m_variableFrame)
				{
					delete m_variableFrame;
				}
				if (m_pMyScope)
				{
					delete m_pMyScope;
				}
			}
			FORCE_INLINE void Fire(int evtIndex,
				X::ARGS& params, X::KWARGS& kwargs)
			{
				if (evtIndex >= 0 && evtIndex < (int)__events.size())
				{
					auto* rt = X::g_pXHost->GetCurrentRuntime();
					__events[evtIndex]->DoFire(rt, nullptr, params, kwargs);
				}
			}
			template<std::size_t Parameter_Num, typename F>
			void AddFunc(const char* func_name, F f)
			{
				m_funclist.push_back(FuncInfo{
					func_name,
					(X::U_FUNC)([f](X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
						X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
						{
							auto* pTContext = dynamic_cast<T*>(pContext);
							auto _retVal = VarCall<Parameter_Num>(pTContext,f,params);
							retValue = X::Value(_retVal);
							return true;
						}) });
			}
			FORCE_INLINE AST::Scope* GetScope()
			{
				return m_pMyScope;
			}
			bool Create()
			{
				m_pMyScope = new AST::Scope();
				m_variableFrame = new AST::StackFrame();
				m_variableFrame->SetVarCount((int)m_funclist.size());
				m_pMyScope->SetVarFrame(m_variableFrame);
				for (auto& f : m_funclist)
				{
					int idx = m_pMyScope->AddOrGet(f.name, false);
					auto* pFuncObj = X::g_pXHost->CreateFunction(f.name.c_str(),f.func);
					X::Value v0(dynamic_cast<X::XObj*>(pFuncObj));
					m_variableFrame->Set(idx, v0);
				}
				m_funclist.clear();//save memory
				return true;
			}
		};
	}
}