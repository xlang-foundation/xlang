#pragma once
#include "singleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "Locker.h"
#include "value.h"
#include "def.h"
#include "wait.h"
#include "utility.h"
#include "xlang.h"
#include "object.h"
#include "attribute.h"
#include "xhost.h"
#include "function.h"
#include <functional>
#include "stackframe.h"
#include "object_scope.h"

namespace X
{
	class Event;
	class XObj;
	class XPackage;
	struct HandlerInfo
	{
		EventHandler Handler = nullptr;
		X::Data::Function* FuncHandler = nullptr;
		int OwnerThreadId = -1;
		long cookie = 0;
	};

	class Event :
		public virtual Data::Object,
		public virtual XEvent
	{
		friend class EventSystem;
		Data::ObjectScope<Event> m_APIs;
		std::string m_name;
		Locker m_lockHandlers;
		long m_lastCookie = 0;
		std::vector<HandlerInfo> m_handlers;
		OnEventHandlerChanged m_changeHandler =nullptr;
	public:
		inline void Fire(int evtIndex,X::ARGS& params, X::KWARGS& kwargs)
		{
			return m_APIs.Fire(evtIndex,params,kwargs);
		}

		Event() :Data::Object(), XObj(), ObjRef(), XEvent()
		{
			m_t = ObjType::Event;
			//auto x0 = typeid(this).name();
			//auto x = typeid(&Event::wait).name();
			m_APIs.AddFunc<2>("wait", &Event::wait);
			m_APIs.Create();
		}
		Event(std::string& name) :Event()
		{
			m_name = name;
		}
		virtual void SetChangeHandler(OnEventHandlerChanged ch) override
		{
			m_changeHandler = ch;
		}
		virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
		{
			bases.push_back(m_APIs.GetScope());
		}
		double wait(int x,double y)
		{
			return x*10.1+y;
		}
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams, X::Value& retValue) override;
		virtual Event& operator +=(X::Value& r) override
		{
			AutoLock(m_lock);
			if (r.IsObject())
			{
				auto* pObjHandler = dynamic_cast<X::Data::Object*>(r.GetObj());
				if (pObjHandler && pObjHandler->GetType() == ObjType::Function)
				{
					auto pFuncHandler = dynamic_cast<X::Data::Function*>(pObjHandler);
					pFuncHandler->AddRef();
					Add(pFuncHandler);
				}
			}
			return *this;
		}
		void CovertPropsToArgs(KWARGS& kwargs)
		{
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				pAttrBag->CovertToDict(kwargs);
			}
		}
		void Fire(X::XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwargs, bool inMain = false)
		{
			int threadId = -1;
			for (auto& k : kwargs)
			{
				if (k.first == "tid")
				{
					threadId = (int)k.second.GetLongLong();
					if (!inMain)
					{//for event excuted in main thread,
					 //need to pass tid, not main thread,
					//don't need to pass
						continue;
					}
				}
				Set(k.first.c_str(), k.second);
			}
			if (inMain)
			{
				FireInMain(rt, pContext, params, kwargs);
			}
			else
			{
				DoFire(rt, pContext, params, kwargs);
			}
		}
		void FireInMain(X::XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwargs);
		virtual void DoFire(XRuntime* rt, XObj* pContext, ARGS& params, KWARGS& kwargs) override
		{
			m_lockHandlers.Lock();
			for (auto& it : m_handlers)
			{
				//if (ownerThreadId ==-1 || it.OwnerThreadId == ownerThreadId)
				{
					IncRef();
					if (it.Handler)
					{
						Value retVal;
						it.Handler(rt, pContext, params, kwargs, retVal);
					}
					else if (it.FuncHandler)
					{
						Value retVal;
						it.FuncHandler->Call(rt, pContext, params, kwargs, retVal);
					}
					DecRef();
				}
			}
			m_lockHandlers.Unlock();
		}
		inline X::Value Get(const char* name)
		{
			std::string strName(name);
			return Get(strName);
		}
		inline X::Value Get(std::string& name)
		{
			X::Value ret;
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				ret = pAttrBag->Get(name);
			}
			return ret;
		}
		inline void Set(const char* name, X::Value& val)
		{
			std::string strName(name);
			Set(strName, val);
		}
		inline void Set(std::string& name, X::Value& val)
		{
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				pAttrBag->Set(name, val);
			}
		}
		inline long Add(EventHandler handler)
		{
			int cnt = 0;
			int tid = (int)GetThreadID();
			m_lockHandlers.Lock();
			long cookie = ++m_lastCookie;
			m_handlers.push_back(HandlerInfo{ handler,nullptr,tid,cookie });
			cnt = (int)m_handlers.size();
			m_lockHandlers.Unlock();
			if (m_changeHandler)
			{
				m_changeHandler(true, cnt);
			}
			return cookie;
		}
		inline long Add(X::Data::Function* pFuncHandler)
		{
			int cnt = 0;
			int tid = (int)GetThreadID();
			m_lockHandlers.Lock();
			long cookie = ++m_lastCookie;
			m_handlers.push_back(HandlerInfo{ nullptr,pFuncHandler,tid,cookie });
			cnt = (int)m_handlers.size();
			m_lockHandlers.Unlock();
			if (m_changeHandler)
			{
				m_changeHandler(true, cnt);
			}
			return cookie;
		}
		inline void Remove(long cookie)
		{
			int cnt = 0;
			m_lockHandlers.Lock();
			auto it = m_handlers.begin();
			while (it != m_handlers.end())
			{//todo: check here
				if ((*it).cookie == cookie)
				{
					m_handlers.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			cnt = (int)m_handlers.size();
			m_lockHandlers.Unlock();
			if (m_changeHandler)
			{
				m_changeHandler(false, cnt);
			}
		}
	};
	class EventSystem :
		public Singleton<EventSystem>
	{
		struct EventFireInfo
		{
			Event* pEvtObj;
			X::XRuntime* rt;
			X::Value valContext;
			ARGS params;
			KWARGS kwParams;
		};
		bool m_run = true;
		Locker m_lockEventOnFire;
		std::vector<EventFireInfo> m_eventsOnFire;

		XWait m_wait;
		Locker m_lockEventMap;
		std::unordered_map<std::string, Event*> m_eventMap;
	public:
		void Shutdown()
		{
			m_run = false;
			m_wait.Release();
		}
		inline void FireInMain(Event* pEvt, XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwParams)
		{
			ARGS params0 = params;//for copy
			KWARGS kwParams0 = kwParams;//for copy
			pEvt->IncRef();
			m_lockEventOnFire.Lock();
			m_eventsOnFire.push_back({ pEvt,rt,Value(pContext),params0,kwParams0 });
			m_lockEventOnFire.Unlock();
			m_wait.Release();
		}
		inline Event* Query(const char* name)
		{
			std::string strName(name);
			return Query(strName);
		}
		inline Event* Query(std::string& name)
		{
			Event* pEvt = nullptr;
			m_lockEventMap.Lock();
			auto it = m_eventMap.find(name);
			if (it != m_eventMap.end())
			{
				pEvt = it->second;
			}
			m_lockEventMap.Unlock();
			if (pEvt)
			{
				pEvt->IncRef();
			}
			return pEvt;
		}
		void Loop()
		{
			while (m_run)
			{
				m_wait.Wait(1000);
				m_lockEventOnFire.Lock();
				while (m_eventsOnFire.size() > 0)
				{
					auto fireInfo = m_eventsOnFire[0];
					m_eventsOnFire.erase(m_eventsOnFire.begin());
					Event* pEvtToRun = fireInfo.pEvtObj;
					m_lockEventOnFire.Unlock();
					pEvtToRun->DecRef();//for m_eventsOnFire
					//todo:
					pEvtToRun->DoFire(fireInfo.rt, fireInfo.valContext.GetObj(),
						fireInfo.params, fireInfo.kwParams);
					m_lockEventOnFire.Lock();
				}
				m_lockEventOnFire.Unlock();
			}
		}
		inline void Fire(X::XRuntime* rt, XObj* pContext,
			std::string& name, ARGS& params, KWARGS& kwargs, bool inMain = false)
		{
			Event* pEvt = Query(name);
			if (pEvt)
			{
				pEvt->Fire(rt, pContext, params, kwargs, inMain);
				pEvt->DecRef();
			}
		}
		inline bool Unregister(const char* name)
		{
			std::string strName(name);
			return Unregister(strName);
		}
		inline bool Unregister(std::string& name)
		{
			Event* pEvt = nullptr;
			m_lockEventMap.Lock();
			auto it = m_eventMap.find(name);
			if (it != m_eventMap.end())
			{
				pEvt = it->second;
				m_eventMap.erase(it);
			}
			m_lockEventMap.Unlock();
			if (pEvt)
			{
				pEvt->DecRef();
			}
			return true;
		}
		inline Event* Register(const char* name)
		{
			std::string strName(name);
			return Register(strName);
		}
		Event* Register(std::string& name)
		{
			Event* pEvt = nullptr;
			m_lockEventMap.Lock();
			auto it = m_eventMap.find(name);
			if (it != m_eventMap.end())
			{
				pEvt = it->second;
			}
			else
			{
				pEvt = new Event();
				pEvt->IncRef();
				pEvt->m_name = name;
				m_eventMap.emplace(std::make_pair(name, pEvt));
			}
			m_lockEventMap.Unlock();
			pEvt->IncRef();
			return pEvt;
		}
	};
}