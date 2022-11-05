#pragma once
#include "xlang.h"
#include "xhost.h"

namespace X
{
	namespace MetaInfo
	{
		template <typename A>
		void process_one_type(std::vector<std::string>& list) {
			list.push_back(typeid(A).name());
		}
		template<typename type, typename...args>
		int GetFuncParamCount(type(*func)(args...))
		{
			return sizeof...(args);
		}
		template<typename type, typename...args>
		std::vector<std::string> BuildFuncInfo(type(*func)(args...))
		{
			std::vector<std::string> list;
			list.push_back(typeid(type).name());
			int placeholder[] = { 0, (process_one_type<args>(list), 0)... };
			(void)placeholder;
			return list;
		}
	}
	template<class impl_pack_class>
	XPackage* RegisterPackage(const char* pack_name, impl_pack_class* instance =nullptr)
	{
		auto& apiset = impl_pack_class::APISET();
		if (apiset.IsCreated())
		{
			return apiset.GetPack();
		}
		impl_pack_class::BuildAPI();
		apiset.Create(instance);

		X::g_pXHost->AddSysCleanupFunc([]() {
			impl_pack_class::APISET().Cleanup();
			});
		if (instance == nullptr)
		{
			X::g_pXHost->RegisterPackage(pack_name, &apiset,[]()
				{
					impl_pack_class* pPackImpl = new impl_pack_class();
					return impl_pack_class::APISET().GetPack();
				});
		}
		else
		{
			auto* pXPack = impl_pack_class::APISET().GetPack();
			X::Value v0(dynamic_cast<X::XObj*>(pXPack));
			X::g_pXHost->RegisterPackage(pack_name,&apiset,v0);
		}
		return apiset.GetPack();
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
		template<typename FirstT,class class_T, typename Array, std::size_t... I>
		inline auto NewClass_impl(FirstT first,Array& a, std::index_sequence<I...>)
		{
			return new class_T(first,a[I]...);
		}
		template<typename FirstT,std::size_t N, class class_T, typename T, typename Indices = std::make_index_sequence<N>>
		inline auto NewClass(FirstT first,T& a)
		{
			return NewClass_impl<FirstT,class_T>(first,a, Indices{});
		}
	}

	class APISetBase
	{
	public:
		enum MemberType
		{
			Func,
			FuncEx,
			Prop,
			ObjectEvent,
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
			std::string doc;
		};
	protected:
		std::vector<MemberInfo> m_members;
		std::vector<int> __events;//index no
		std::vector<APISetBase*> m_bases;
		XPackage* m_xPack = nullptr;
		bool m_alreadyCallBuild = false;
	public:
		void CollectBases(std::vector<APISetBase*>& bases)
		{
			for (auto* p : m_bases)
			{
				p->CollectBases(bases);
			}
			//derived class's same name member will override base's
			bases.push_back(this);
		}
		inline auto& Members() { return m_members; }
		virtual XPackage* GetPack() = 0;
		virtual XPackage* GetProxy(void* pRealObj) = 0;
	};
	template<class T>
	class XPackageAPISet:
		public APISetBase
	{
	public:
		bool IsCreated() { return m_alreadyCallBuild; }
		inline virtual XPackage* GetPack() override { return m_xPack; }
		inline virtual XPackage* GetProxy(void* pRealObj) override
		{
			T* pTObj = (T*)pRealObj;
			if (pTObj->__pPack_)
			{
				return pTObj->__pPack_;
			}
			auto* pPack =  g_pXHost->CreatePackageProxy(m_xPack, pRealObj);
			pTObj->__pPack_ = pPack;
			return pPack;
		}
		void Cleanup()
		{
			if (m_xPack)
			{
				m_xPack->RemoveALl();
				m_xPack->DecRef();
				m_xPack = nullptr;
			}
		}
		~XPackageAPISet()
		{
			if (m_xPack)
			{
				m_xPack->DecRef();
			}
		}
		inline void AddBase(APISetBase* pBase)
		{
			m_bases.push_back(pBase);
		}
		inline void Fire(XPackage* pPack,int evtIndex,
			X::ARGS& params, X::KWARGS& kwargs)
		{
			if (pPack && evtIndex >= 0 && evtIndex < (int)__events.size())
			{
				auto* rt = X::g_pXHost->GetCurrentRuntime();
				X::Value vEvt;
				pPack->GetIndexValue(__events[evtIndex], vEvt);
				X::XEvent* pEvt = dynamic_cast<X::XEvent*>(vEvt.GetObj());
				pEvt->DoFire(rt, nullptr, params, kwargs);
			}
		}
		inline X::XEvent* GetEvent(XPackage* pPack, int evtIndex)
		{
			X::XEvent* pEvt = nullptr;
			if (pPack && evtIndex >= 0 && evtIndex < (int)__events.size())
			{
				auto* rt = X::g_pXHost->GetCurrentRuntime();
				X::Value vEvt;
				pPack->GetIndexValue(__events[evtIndex], vEvt);
				pEvt = dynamic_cast<X::XEvent*>(vEvt.GetObj());
				pEvt->IncRef();
			}
			return pEvt;
		}
		void AddEvent(const char* name)
		{
			m_members.push_back(MemberInfo{ MemberType::ObjectEvent,name });
		}
		template<std::size_t Parameter_Num, class Class_T>
		void AddClass(const char* class_name, Class_T* class_inst = nullptr,
			PackageCleanup cleanFunc = nullptr)
		{
			auto& apiset = Class_T::APISET();
			Class_T::BuildAPI();
			apiset.Create(class_inst, cleanFunc);

			m_members.push_back(MemberInfo{
				MemberType::Class,class_name,
				(X::U_FUNC)([class_inst](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						Class_T* cls = nullptr;
						if (class_inst == nullptr)
						{
							cls = HelpFuncs::NewClass<Parameter_Num, Class_T>(params);
						}
						else
						{
							cls = class_inst;
						}
						retValue = X::Value(Class_T::APISET().GetProxy(cls),false);
						return true;
					}),nullptr });
		}
		template<std::size_t Parameter_Num, class Class_T, class Parent_T>
		void AddClass(const char* class_name, Class_T* class_inst = nullptr,
			PackageCleanup cleanFunc = nullptr)
		{
			auto& apiset = Class_T::APISET();
			Class_T::BuildAPI();
			apiset.Create(class_inst, cleanFunc);

			m_members.push_back(MemberInfo{
				MemberType::Class,class_name,
				(X::U_FUNC)([class_inst](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						Class_T* cls = nullptr;
						if (class_inst == nullptr)
						{
							Parent_T* pParentObj = nullptr;
							if (pContext != nullptr)
							{
								XPackage* pParentPack = dynamic_cast<XPackage*>(pContext);
								if (pParentPack)
								{
									pParentObj = (Parent_T*)pParentPack->GetEmbedObj();
								}
							}
							cls = HelpFuncs::NewClass<Parent_T*,Parameter_Num, Class_T>(pParentObj,params);
						}
						else
						{
							cls = class_inst;
						}
						retValue = X::Value(Class_T::APISET().GetProxy(cls),false);
						return true;
					}),nullptr });
		}
		template<std::size_t Parameter_Num, typename F>
		void AddFunc(const char* func_name, F f,const char* doc = "")
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
					}),nullptr,nullptr,false,std::string(doc)});
		}
		template<std::size_t Parameter_Num, typename F>
		void AddFuncEx(const char* func_name, F f, const char* doc = "")
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
					}),nullptr,nullptr,false,std::string(doc)});
		}
		template<std::size_t Parameter_Num, typename F>
		void AddRTFunc(const char* func_name, F f, const char* doc = "")
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
					}),nullptr,nullptr,false,std::string(doc) });
		}
		template<typename F>
		void AddVarFuncEx(const char* func_name, F f, const char* doc = "")
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
					}),false,std::string(doc) });
		}
		template<typename F>
		void AddRawParamFunc(const char* func_name, F f, const char* doc = "")
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
					}),true,std::string(doc) });
		}
		template<typename F>
		void AddVarFunc(const char* func_name, F f, const char* doc = "")
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
					}),nullptr,nullptr,false,std::string(doc) });
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
		//in gcc, id PTMV is std::string will make error
		//with ambiguous overload for ‘operator=’ 
		//so use this way to add type
		template<typename VAR_TYPE,typename PTMV>
		void AddPropWithType(const char* func_name, PTMV var)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				(X::U_FUNC)([var](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						VAR_TYPE _v = params[0];
						(pThis->*var) = _v;
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
		void AddPropL(const char* func_name,std::function<void(T* pThis,X::Value)> setF,
			std::function<X::Value(T* pThis)> getF)
		{
			m_members.push_back(MemberInfo{
				MemberType::Prop,func_name,
				(X::U_FUNC)([setF](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						setF(pThis,params[0]);
						retValue = X::Value(true);
						return true;
					}),
				(X::U_FUNC)([getF](X::XRuntime* rt,X::XObj* pContext,
					X::ARGS& params,X::KWARGS& kwParams,X::Value& retValue)
					{
						auto* pPackage = dynamic_cast<X::XPackage*>(pContext);
						auto* pThis = (T*)pPackage->GetEmbedObj();
						retValue = getF(pThis);
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
		bool Create(T* thisObj, PackageCleanup cleanFunc = nullptr)
		{
			if (m_alreadyCallBuild)
			{
				return true;
			}
			std::vector<APISetBase*> bases;
			CollectBases(bases);//include this class also
			int memberNum = 0;
			for (auto* b : bases)
			{
				memberNum += (int)b->Members().size();;
			}

			auto* pPackage = X::g_pXHost->CreatePackage(this,thisObj);
			//if object created by outside,thisObj will not be NULL
			//then don't need to be deleted by XPackage
			if (thisObj == nullptr)
			{
				if (cleanFunc == nullptr)
				{
					cleanFunc = [](void* pObj) {
						delete (T*)pObj;
					};
				}
				pPackage->SetPackageCleanupFunc(cleanFunc);
			}
			pPackage->Init(memberNum);
			for (auto* b : bases) 
			{
				for (auto& m : b->Members())
				{
					int idx = pPackage->AddMethod(m.name.c_str(), m.keepRawParams);
					X::Value v0;
					switch (m.type)
					{
					case MemberType::Class:
					case MemberType::Func:
					{
						auto* pObjFun = dynamic_cast<X::XObj*>(pPackage);
						auto* pFuncObj = X::g_pXHost->CreateFunction(m.name.c_str(), m.func, pObjFun);
						v0 = X::Value(pFuncObj, false);
					}
					break;
					case MemberType::FuncEx:
					{
						auto* pObjFun = dynamic_cast<X::XObj*>(pPackage);
						auto* pFuncObj = X::g_pXHost->CreateFunctionEx(m.name.c_str(), m.func_ex, pObjFun);
						v0 = X::Value(pFuncObj, false);
					}
					break;
					case MemberType::Prop:
					{
						auto* pPropObj = X::g_pXHost->CreateProp(m.name.c_str(), m.func, m.func2);
						v0 = X::Value(pPropObj, false);
					}
					break;
					case MemberType::ObjectEvent:
					{
						auto* pEvtObj = X::g_pXHost->CreateXEvent(m.name.c_str());
						v0 = X::Value(pEvtObj, false);
						__events.push_back(idx);
					}
					default:
						break;
					}
					pPackage->SetIndexValue(idx, v0);
				}
			}
			m_xPack = pPackage;
			m_alreadyCallBuild = true;
			return true;
		}
	};
}

#define ADD_BASE(CLS_T)\
	CLS_T::BuildAPI();\
	APISET().AddBase(&CLS_T::APISET());
#define BEGIN_PACKAGE(class_name)\
public:\
	static X::XPackageAPISet<class_name>& APISET()\
	{\
		static 	X::XPackageAPISet<class_name> _Apis;\
		return _Apis;\
	}\
	static void BuildAPI()\
	{\
		static bool __called_ = false;\
		if(__called_) return;\
		__called_ = true;
#define ADD_FUNC(parameter_num,name,Func)\
	APISET().AddFunc<parameter_num>(name,Func);
#define ADD_EVENT(name) APISET().AddEvent(#name);

#define END_PACKAGE\
	}\
X::XPackage* __pPack_ = nullptr;\
void Fire(int evtIndex, X::ARGS& params, X::KWARGS& kwargs)\
{\
	APISET().Fire(__pPack_,evtIndex, params, kwargs);\
}\
X::XEvent* GetEvent(int evtIndex)\
{\
	return APISET().GetEvent(__pPack_, evtIndex);\
}
