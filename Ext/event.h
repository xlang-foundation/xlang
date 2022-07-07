#pragma once
#include "singleton.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "Locker.h"
#include "value.h"
#include "def.h"
#include "wait.h"
namespace X
{
	class Event;
	typedef void (*EventHandler)(void* pContext,Event* pEvt);
	struct HandlerInfo
	{
		EventHandler Handler;
		void* Context;

	};
	class Event
	{
		friend class EventSystem;
		std::string m_name;
		Locker m_lockHandlers;
		std::vector<HandlerInfo> m_handlers;
		Locker m_lockProps;
		std::unordered_map<std::string, AST::Value> m_props;
	public:
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
			for (auto& k : kwargs)
			{
				Set(k.first.c_str(), k.second);
			}
			if (inMain)
			{
				FireInMain();
			}
			else
			{
				Fire();
			}
		}
		void FireInMain();
		void Fire()
		{
			m_lockHandlers.Lock();
			for (auto& it : m_handlers)
			{
				it.Handler(it.Context, this);
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
		inline void Add(EventHandler handler, void* pContext)
		{
			m_lockHandlers.Lock();
			m_handlers.push_back(HandlerInfo{ handler,pContext });
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
			}
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
				pEvt->m_name = name;
				m_eventMap.emplace(std::make_pair(name, pEvt));
			}
			m_lockEventMap.Unlock();
			return pEvt;
		}
	};
}