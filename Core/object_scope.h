#pragma once

namespace X
{
	namespace Data 
	{
		template<class class_T, typename F, typename Array, std::size_t... I>
		inline auto VarCall_impl(class_T* pThis, F f, Array& a, std::index_sequence<I...>)
		{
			return (pThis->*f)(a[I]...);
		}
		template<std::size_t N, class class_T, typename F, typename T, typename Indices = std::make_index_sequence<N>>
		inline auto VarCall(class_T* pThis, F f, T& a)
		{
			return VarCall_impl(pThis, f, a, Indices{});
		}

		template<class T>
		class ObjectScope :
			virtual public AST::Scope
		{
		protected:
			struct FuncInfo
			{
				std::string name;
				X::U_FUNC func;
			};
			std::vector<FuncInfo> m_funclist;
			AST::StackFrame* m_stackFrame = nullptr;
			std::vector<X::XEvent*> __events;
			virtual Scope* GetParentScope() override { return nullptr; }
			// Inherited via Scope
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
		public:
			~ObjectScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			inline void Fire(int evtIndex,
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
					(X::U_FUNC)([f](X::XRuntime* rt,X::XObj* pContext,
						X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
						{
							auto* pThis = dynamic_cast<T*>(pContext);
							auto _retVal = VarCall<Parameter_Num>(pThis,f,params);
							retValue = X::Value(_retVal);
							return true;
						}) });
			}
			inline AST::Scope* GetScope()
			{
				return dynamic_cast<AST::Scope*>(this);
			}
			bool Create()
			{
				m_stackFrame = new AST::StackFrame(this);
				m_stackFrame->SetVarCount((int)m_funclist.size());
				for (auto& f : m_funclist)
				{
					int idx = AddOrGet(f.name, false);
					auto* pFuncObj = X::g_pXHost->CreateFunction(f.name.c_str(),
						(X::U_FUNC)f.func);
					X::Value v0(dynamic_cast<X::XObj*>(pFuncObj));
					m_stackFrame->Set(idx, v0);
				}
				m_funclist.clear();//save memory
				return true;
			}
		};
	}
}