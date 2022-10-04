#pragma once
#include "xlang.h"
#include "xhost.h"

namespace X
{
	template<class impl_pack_class>
	void RegisterPackage(const char* pack_name, impl_pack_class* instance =nullptr)
	{
		if (instance == nullptr)
		{
			X::g_pXHost->RegisterPackage(pack_name, []()
				{
					impl_pack_class* pPackImpl = new impl_pack_class(); \
						return pPackImpl->APISET().GetPack();
				});
		}
		else
		{
			auto* pXPack = instance->APISET().GetPack();
			X::Value v0(dynamic_cast<X::XObj*>(pXPack));
			X::g_pXHost->RegisterPackage(pack_name, v0);
		}
	}

	namespace HelpFuncs
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
		template<class class_T, typename F, typename Array, std::size_t... I>
		inline auto VarCall_Ex_impl(class_T* pThis, F f, X::Value& extra, Array& a, std::index_sequence<I...>)
		{
			return (pThis->*f)(extra,a[I]...);
		}
		template<std::size_t N, class class_T, typename F, typename T, typename Indices = std::make_index_sequence<N>>
		inline auto VarCallEx(class_T* pThis, F f,X::Value& extra,T& a)
		{
			return VarCall_Ex_impl(pThis, f, extra,a, Indices{});
		}

		template<class class_T, typename F, typename Array, std::size_t... I>
		inline auto VarCall_impl_Extra(X::XRuntime* rt, X::XObj* pContext,
			class_T* pThis, F f, Array& a, std::index_sequence<I...>)
		{
			return (pThis->*f)(rt, pContext,a[I]...);
		}
		template<std::size_t N, class class_T, typename F, typename T, typename Indices = std::make_index_sequence<N>>
		inline auto VarCall_Extra(X::XRuntime* rt, X::XObj* pContext,class_T* pThis, F f, T& a)
		{
			return VarCall_impl_Extra(rt,pContext,pThis, f, a, Indices{});
		}

		template<class class_T, typename Array, std::size_t... I>
		inline auto NewClass_impl(Array& a, std::index_sequence<I...>)
		{
			return new class_T(a[I]...);
		}
		template<std::size_t N, class class_T, typename T, typename Indices = std::make_index_sequence<N>>
		inline auto NewClass(T& a)
		{
			return NewClass_impl<class_T>(a, Indices{});
		}
	}
	template<class T>
	class XPackageAPISet
	{
	protected:
		enum MemberType
		{
			Func,
			FuncEx,
			Prop,
			Event,
			Class,
			ClassInstance,
		};
		struct MemberInfo
		{
			MemberType type;
			std::string name;
			X::U_FUNC func;
			X::U_FUNC func2;
			X::U_FUNC_EX func_ex;
			bool keepRawParams = false;
		};
		std::vector<MemberInfo> m_members;
		std::vector<X::XEvent*> __events;
		XPackage* m_xPack = nullptr;
	public:
		XPackage* GetPack() { return m_xPack; }
		X::XEvent* GetEvent(int idx) { return __events[idx]; }
		~XPackageAPISet()
		{
			if (m_xPack)
			{
				m_xPack->DecRef();
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
		void AddEvent(const char* name)
		{
			m_members.push_back(MemberInfo{ MemberType::Event,name });
		}
		template<std::size_t Parameter_Num, class T>
		void AddClass(const char* class_name,T* class_inst =nullptr)
		{
			m_members.push_back(MemberInfo{
				MemberType::Class,class_name,
				(X::U_FUNC)([class_inst](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						T* cls = nullptr;
						if (class_inst == nullptr)
						{
							cls = HelpFuncs::NewClass<Parameter_Num,T>(params);
						}
						else
						{
							cls = class_inst;
						}
						retValue = X::Value(cls->APISET().GetPack());
						return true;
					}),nullptr });
		}
		template<std::size_t Parameter_Num, typename F>
		void AddFunc(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::Func,func_name,
				(X::U_FUNC)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						auto _retVal = HelpFuncs::VarCall<Parameter_Num>(pThis,f,params);
						retValue = X::Value(_retVal);
						return true;
					}),nullptr });
		}
		template<std::size_t Parameter_Num, typename F>
		void AddFuncEx(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::FuncEx,func_name,nullptr,nullptr,
				(X::U_FUNC_EX)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& trailer,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						auto _retVal = HelpFuncs::VarCallEx<Parameter_Num>(pThis,f, trailer,params);
						retValue = X::Value(_retVal);
						return true;
					})});
		}
		template<std::size_t Parameter_Num, typename F>
		void AddRTFunc(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::Func,func_name,
				(X::U_FUNC)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						auto _retVal = HelpFuncs::VarCall_Extra<Parameter_Num>(rt, pContext,pThis,f,params);
						retValue = X::Value(_retVal);
						return true;
					}),nullptr });
		}
		template<typename F>
		void AddVarFuncEx(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::FuncEx,func_name,nullptr,nullptr,
				(X::U_FUNC_EX)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& trailer,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						(pThis->*f)(rt, pContext, params, kwParams, trailer, retValue);
						return true;
					})});
		}
		template<typename F>
		void AddRawParamFunc(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::FuncEx,func_name,nullptr,nullptr,
				(X::U_FUNC_EX)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& trailer,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						(pThis->*f)(rt, pContext, params, kwParams, trailer, retValue);
						return true;
					}),true});
		}
		template<typename F>
		void AddVarFunc(const char* func_name, F f)
		{
			m_members.push_back(MemberInfo{
				MemberType::Func,func_name,
				(X::U_FUNC)([f](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						(pThis->*f)(rt, pContext, params, kwParams, retValue);
						return true;
					}),nullptr });
		}
		template<typename PTMV>
		void AddProp0(const char* func_name, PTMV var)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				(X::U_FUNC)([var](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						(pThis->*var) = params[0];
						retValue = X::Value(true);
						return true;
					}),
				(X::U_FUNC)([var](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						retValue = X::Value(pThis->*var);
						return true;
					})
				});
		}
		void AddPropL(const char* func_name,std::function<void(X::Value)> setF,
			std::function<X::Value()> getF)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				(X::U_FUNC)([setF](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						setF(params[0]);
						retValue = X::Value(true);
						return true;
					}),
				(X::U_FUNC)([getF](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						retValue = getF();
						return true;
					})
				});
		}
		template<typename GETF>
		void AddProp(const char* func_name,GETF get)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				nullptr,
				(X::U_FUNC)([get](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						retValue = X::Value((pThis->*get)());
						return true;
					})
				});
		}
		template<typename SETF, typename GETF>
		void AddProp(const char* func_name, SETF set, GETF get)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				(X::U_FUNC)([set](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						(pThis->*set)(params[0]);
						retValue = X::Value(true);
						return true;
					}),
				(X::U_FUNC)([get](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						retValue = X::Value((pThis->*get)());
						return true;
					})
				});
		}
		bool Create(T* thisObj)
		{
			auto* pPackage = X::g_pXHost->CreatePackage(thisObj);
			pPackage->Init((int)m_members.size());
			for (auto& m : m_members)
			{
				int idx = pPackage->AddMethod(m.name.c_str(),m.keepRawParams);
				X::Value v0;
				switch (m.type)
				{
				case MemberType::Class:
				case MemberType::Func:
				{
					auto* pObjFun = dynamic_cast<X::XObj*>(pPackage);
					auto* pFuncObj = X::g_pXHost->CreateFunction(m.name.c_str(), m.func, pObjFun);
					v0 = dynamic_cast<X::XObj*>(pFuncObj);
				}
				break;
				case MemberType::FuncEx:
				{
					auto* pObjFun = dynamic_cast<X::XObj*>(pPackage);
					auto* pFuncObj = X::g_pXHost->CreateFunctionEx(m.name.c_str(), m.func_ex, pObjFun);
					v0 = dynamic_cast<X::XObj*>(pFuncObj);
				}
				break;
				case MemberType::Prop:
				{
					auto* pPropObj = X::g_pXHost->CreateProp(m.name.c_str(), m.func, m.func2);
					v0 = dynamic_cast<X::XObj*>(pPropObj);
				}
				break;
				case MemberType::Event:
				{
					auto* pEvtObj = X::g_pXHost->CreateXEvent(m.name.c_str());
					v0 = dynamic_cast<X::XObj*>(pEvtObj);
					__events.push_back(pEvtObj);
				}
				default:
					break;
				}
				pPackage->SetIndexValue(idx, v0);
			}
			m_members.clear();//save memory
			m_xPack = pPackage;
			return true;
		}
	};
}
#define BEGIN_PACKAGE(class_name)\
	X::XPackageAPISet<class_name> m_Apis;\
public:\
	X::XPackageAPISet<class_name>& APISET() { return m_Apis; }\
	void RegisterAPIS()\
	{

#define ADD_FUNC(parameter_num,name,Func)\
	m_Apis.AddFunc<parameter_num>(name,Func);
#define ADD_EVENT(name) m_Apis.AddEvent(#name);

#define END_PACKAGE\
	m_Apis.Create(this);\
	}
