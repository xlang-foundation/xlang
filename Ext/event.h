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

namespace X
{
	class Event;
	typedef void (*EventHandler)(void* pContext, void* pContext2,Event* pEvt);
	struct HandlerInfo
	{
		EventHandler Handler=nullptr;
		void* Context =nullptr;
		void* Context2 = nullptr;
		int OwnerThreadId = -1;
	};
	class Event:
		virtual public Data::Object
	{
		friend class EventSystem;
		std::string m_name;
		Locker m_lockHandlers;
		std::vector<HandlerInfo> m_handlers;
	public:
		void CovertPropsToArgs(KWARGS& kwargs)
		{
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				pAttrBag->CovertToDict(kwargs);
			}
		}
		void Fire(KWARGS& kwargs, bool inMain =false)
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
				FireInMain();
			}
			else
			{
				Fire(threadId);
			}
		}
		void FireInMain();
		void Fire(int ownerThreadId=-1)
		{
			m_lockHandlers.Lock();
			for (auto& it : m_handlers)
			{
				//if (ownerThreadId ==-1 || it.OwnerThreadId == ownerThreadId)
				{
					IncRef();
					it.Handler(it.Context, it.Context2, this);
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
				pAttrBag->Set(name,val);
			}
		}
		inline void* Add(EventHandler handler, void* pContext, void* pContext2)
		{
			int tid = (int)GetThreadID();
			m_lockHandlers.Lock();
			m_handlers.push_back(HandlerInfo{ handler,pContext,pContext2,tid});
			m_lockHandlers.Unlock();
			return (void*)handler;
		}
		inline void Remove(void* handler)
		{
			m_lockHandlers.Lock();
			auto it = m_handlers.begin();
			while (it != m_handlers.end())
			{
				if ((*it).Handler == handler)
				{
					m_handlers.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			m_lockHandlers.Unlock();
		}
	};
	class EventSystem :
		public Singleton<EventSystem>
	{
		bool m_run = true;
		Locker m_lockEventOnFire;
		std::vector<Event*> m_eventsOnFire;

		XWait m_wait;
		Locker m_lockEventMap;
		std::unordered_map<std::string, Event*> m_eventMap;
	public:
		void Shutdown()
		{
			m_run = false;
			m_wait.Release();
		}
		inline void FireInMain(Event* pEvt)
		{
			pEvt->IncRef();
			m_lockEventOnFire.Lock();
			m_eventsOnFire.push_back(pEvt);
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
					Event* pEvtToRun = m_eventsOnFire[0];
					m_eventsOnFire.erase(m_eventsOnFire.begin());
					m_lockEventOnFire.Unlock();
					pEvtToRun->DecRef();//for m_eventsOnFire
					pEvtToRun->Fire();
					m_lockEventOnFire.Lock();
				}
				m_lockEventOnFire.Unlock();
			}
		}
		inline void Fire(std::string& name, 
			KWARGS& kwargs,bool inMain=false)
		{
			Event* pEvt = Query(name);
			if (pEvt)
			{
				pEvt->Fire(kwargs, inMain);
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
				pEvt =it->second;
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