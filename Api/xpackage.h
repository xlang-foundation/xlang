#pragma once
#include "xlang.h"
#include "xhost.h"

namespace X
{
	template<class impl_pack_class>
	void RegisterPackage(const char* pack_name) 
	{
		X::g_pXHost->RegisterPackage(pack_name, []()
			{
				impl_pack_class* pPackImpl = new impl_pack_class(); \
				return pPackImpl->APISET().GetPack();
			});
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
		};
		std::vector<MemberInfo> m_members;
		std::vector<X::XEvent*> __events;
		XPackage* m_xPack = nullptr;
	public:
		XPackage* GetPack() { return m_xPack; }
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
				int idx = pPackage->AddMethod(m.name.c_str());
				X::Value v0;
				switch (m.type)
				{
				case MemberType::Class:
				case MemberType::Func:
				{
					auto* pFuncObj = X::g_pXHost->CreateFunction(m.name.c_str(), m.func);
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
#if 0
#define REGISTER_PACKAGE(pack_name,impl_pack_name)\
	X::g_pXHost->RegisterPackage(pack_name, []()\
	{\
		impl_pack_name* pPackImpl = new impl_pack_name();\
		X::XPackage* pPackage = nullptr;\
		pPackImpl->Create(&pPackage);\
		return pPackage;\
	});

#define REGISTER_PACKAGE(pack_name,impl_pack_name)\
	X::g_pXHost->RegisterPackage(pack_name, []()\
	{\
		impl_pack_name* pPackImpl = new impl_pack_name();\
		X::XPackage* pPackage = nullptr;\
		pPackImpl->Create(&pPackage);\
		return pPackage;\
	});

#define REGISTER_OBJ_PACKAGE(pack_name,valObj)\
	X::g_pXHost->RegisterPackage(pack_name, valObj);

#define BEGIN_PACKAGE(class_name) \
		typedef class_name THIS_CLASS_NAME;\
		X::XPackage* __xpack =nullptr;\
		inline X::XPackage* get_xpack() {return __xpack;}\
		std::vector<X::XEvent*> __events;\
		inline void Fire(int evtIndex,\
			X::ARGS& params, X::KWARGS& kwargs)\
		{\
			if (evtIndex >= 0 && evtIndex < (int)__events.size())\
			{\
				auto* rt = X::g_pXHost->GetCurrentRuntime();\
				__events[evtIndex]->DoFire(rt,nullptr,params,kwargs);\
			}\
		}\
		bool Create(X::XPackage** ppackage)\
		{\
			std::vector<std::pair<int,X::XObj*>> _members_;\
			auto* pPackage = X::g_pXHost->CreatePackage(this);\
			__xpack = pPackage;

#define DEF_U_FUNC_HEAD(fn_name)\
	{\
	int idx = pPackage->AddMethod(fn_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(fn_name,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();

#define DEF_U_FUNC_TAIL()\
			retValue = X::Value(_retVal);\
			return true;\
		}));\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_PROP4(prop_name_out,prop_name_inner,op_before,op_after,conv)\
	{\
	int idx = pPackage->AddMethod(prop_name_out);\
	auto* pPropObj = X::g_pXHost->CreateProp(prop_name_out,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->op_before;\
			pThis->prop_name_inner = pThis->conv(params[0]); \
			pThis->op_after;\
			return true; \
		}),\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->op_before;\
			retValue = X::Value(pThis->conv(pThis->prop_name_inner));\
			pThis->op_after;\
			return true; \
		})\
		);\
	_members_.push_back(std::make_pair(idx, pPropObj)); \
	}

#define ADD_PROP3(prop_name_out,prop_name_inner,op_before,op_after)\
	{\
	int idx = pPackage->AddMethod(prop_name_out);\
	auto* pPropObj = X::g_pXHost->CreateProp(prop_name_out,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->op_before;\
			pThis->prop_name_inner = params[0]; \
			pThis->op_after;\
			return true; \
		}),\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->op_before;\
			retValue = X::Value(pThis->prop_name_inner);\
			pThis->op_after;\
			return true; \
		})\
		);\
	_members_.push_back(std::make_pair(idx, pPropObj)); \
	}

#define ADD_PROP2(prop_name_out,prop_name_inner)\
	{\
	int idx = pPackage->AddMethod(prop_name_out);\
	auto* pPropObj = X::g_pXHost->CreateProp(prop_name_out,\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			pThis->prop_name_inner = params[0]; \
			return true; \
		}),\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			retValue = X::Value(pThis->prop_name_inner);\
			return true; \
		})\
		);\
	_members_.push_back(std::make_pair(idx, pPropObj)); \
	}

#define ADD_PROP(prop_name) ADD_PROP2(#prop_name, prop_name)

#define ADD_FUNC0(fn_name,function_name,retType)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name();\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC1(fn_name,function_name,retType,p1)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0]);\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC2(fn_name,function_name,retType,p1,p2)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0],(p2)params[1]);\
	DEF_U_FUNC_TAIL()

#define ADD_FUNC3(fn_name,function_name,retType,p1,p2,p3)\
	DEF_U_FUNC_HEAD(fn_name)\
	retType _retVal = pThis->function_name((p1)params[0],\
			(p2)params[1],(p3)params[2]);\
	DEF_U_FUNC_TAIL()



#define DEF_U_FUNC_XLANG_STYLE_CALL(function_name)\
	(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
		X::ARGS& params,\
		X::KWARGS& kwParams,\
		X::Value& retValue)\
		{\
			auto* pPackage = dynamic_cast<X::XPackage*>(pContext);\
			auto* pThis = (THIS_CLASS_NAME*)pPackage->GetEmbedObj();\
			return pThis->function_name(rt,pContext,params, kwParams, retValue);\
		})

#define ADD_FUNC(fn_name,function_name)\
	{\
	int idx = pPackage->AddMethod(fn_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(fn_name,\
			DEF_U_FUNC_XLANG_STYLE_CALL(function_name)\
			);\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_EVENT(name)\
	{\
	int idx = pPackage->AddMethod(#name);\
	auto* pEvtObj = X::g_pXHost->CreateXEvent(#name);\
	__events.push_back(pEvtObj);\
	_members_.push_back(std::make_pair(idx, pEvtObj)); \
	}


#define DEF_CLASS_HEAD(class_name)\
	{\
	int idx= pPackage->AddMethod(class_name);\
	auto* pFuncObj = X::g_pXHost->CreateFunction(class_name,\
		(X::U_FUNC)([](X::XRuntime* rt, X::XObj* pContext,\
			X::ARGS& params,\
			X::KWARGS& kwParams,\
			X::Value& retValue)\
			{
#define DEF_CLASS_TAIL()\
				X::XPackage* pPackage_Srv = nullptr;\
				cls->Create(&pPackage_Srv);\
				retValue = X::Value(pPackage_Srv);\
				return true;\
			}));\
	_members_.push_back(std::make_pair(idx, pFuncObj));\
	}

#define ADD_CLASS_INSTANCE(class_name,class_inst)\
	DEF_CLASS_HEAD(class_name)\
		auto* cls = class_inst;\
	DEF_CLASS_TAIL()

#define ADD_CLASS(class_name,class_impl_name)\
	DEF_CLASS_HEAD(class_name)\
		auto* cls = new class_impl_name(params,kwParams);\
	DEF_CLASS_TAIL()

#define END_PACKAGE \
	pPackage->Init((int)_members_.size());\
	for(auto& it:_members_)\
	{\
		X::Value v0(it.second);\
		pPackage->SetIndexValue(it.first, v0);\
	}\
	*ppackage = pPackage;\
	return true;\
	}
#endif
