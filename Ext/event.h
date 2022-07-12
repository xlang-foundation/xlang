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

namespace X
{
	class Event;
	typedef void (*EventHandler)(void* pContext,Event* pEvt);
	struct HandlerInfo
	{
		EventHandler Handler=nullptr;
		void* Context =nullptr;
		int OwnerThreadId = -1;
	};
	class Event
	{
		friend class EventSystem;
		Locker m_lock;
		int m_ref = 0;
		std::string m_name;
		Locker m_lockHandlers;
		std::vector<HandlerInfo> m_handlers;
		Locker m_lockProps;
		std::unordered_map<std::string, AST::Value> m_props;
	public:
		int AddRef()
		{
			m_lock.Lock();
			int ref = ++m_ref;
			m_lock.Unlock();
			return ref;
		}
		int Release()
		{
			m_lock.Lock();
			if (--m_ref == 0)
			{
				m_lock.Unlock();
				delete this;
				return 0;
			}
			else
			{
				m_lock.Unlock();
				return m_ref;
			}
		}
		void CovertPropsToArgs(KWARGS& kwargs)
		{
			m_lockProps.Lock();
			for (auto& it : m_props)
			{
				kwargs.emplace(it);
			}
			m_lockProps.Unlock();
		}
		void Fire(KWARGS& kwargs, bool inMain =false)
		{
			int threadId = 0;
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
				if (ownerThreadId ==-1 || it.OwnerThreadId == ownerThreadId)
				{
					AddRef();
					it.Handler(it.Context, this);
					Release();
				}
			}
			m_lockHandlers.Unlock();
		}
		inline AST::Value Get(const char* name)
		{
			std::string strName(name);
			return Get(strName);
		}
		inline AST::Value Get(std::string& name)
		{
			AST::Value ret;
			m_lockProps.Lock();
			auto it = m_props.find(name);
			if (it != m_props.end())
			{
				ret = it->second;
			}
			m_lockProps.Unlock();
			return ret;
		}
		inline void Set(const char* name, AST::Value& val)
		{
			std::string strName(name);
			Set(strName, val);
		}
		inline void Set(std::string& name, AST::Value& val)
		{
			m_lockProps.Lock();
			auto it = m_props.find(name);
			if (it != m_props.end())
			{
				it->second = val;
			}
			else
			{
				m_props.emplace(std::make_pair(name, val));
			}
			m_lockProps.Unlock();
		}
		inline void* Add(EventHandler handler, void* pContext)
		{
			int tid = (int)GetThreadID();
			m_lockHandlers.Lock();
			m_handlers.push_back(HandlerInfo{ handler,pContext,tid});
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
			pEvt->AddRef();
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
				pEvt->AddRef();
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
					pEvtToRun->Release();//for m_eventsOnFire
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
				pEvt->Release();
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
				pEvt->Release();
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
				pEvt->AddRef();
				pEvt->m_name = name;
				m_eventMap.emplace(std::make_pair(name, pEvt));
			}
			m_lockEventMap.Unlock();
			pEvt->AddRef();
			return pEvt;
		}
	};
}