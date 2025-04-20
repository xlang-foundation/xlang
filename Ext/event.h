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
#include "remote_object.h"
#include "object_scope.h"
#include "wait.h"
#if (WIN32)
#include <conio.h>
#endif
namespace X
{
	class ObjectEvent;
	class XObj;
	class XPackage;
	struct HandlerInfo
	{
		EventHandler Handler;//for native handler
		X::Data::Object* ObjectHandler = nullptr;
		int OwnerThreadId = -1;
		long cookie = 0;
	};
	using EventTask = std::function<void(ARGS& params)>;
	class ObjectEvent :
		public virtual Data::Object,
		public virtual XEvent
	{
		friend class EventSystem;
		Data::ObjectScope<ObjectEvent> m_APIs;
		std::string m_name;
		Locker m_lockHandlers;
		long m_lastCookie = 0;
		std::vector<HandlerInfo> m_handlers;
		OnEventHandlerChanged m_changeHandler;
		Locker m_statusLock;
		std::vector<XWait*> m_waits;
		bool m_fired = false;
	public:
		FORCE_INLINE void Fire(int evtIndex, X::ARGS& params, X::KWARGS& kwargs)
		{
			SetFire();
			m_APIs.Fire(evtIndex, params, kwargs);
		}
		FORCE_INLINE virtual bool wait(int timeout) override
		{
			return WaitOn(timeout);
		}
		ObjectEvent() :Data::Object(), XObj(), ObjRef(), XEvent()
		{
			m_t = ObjType::ObjectEvent;
			m_APIs.AddFunc<1>("wait", &ObjectEvent::WaitOn);
			m_APIs.Create();
		}
		ObjectEvent(std::string& name) :ObjectEvent()
		{
			m_name = name;
		}
		virtual const char* ToString(bool WithFormat = false)
		{
			std::string retVal;
			for (auto& handleInfo : m_handlers)
			{
				if (handleInfo.ObjectHandler)
				{
					auto str_abi = handleInfo.ObjectHandler->ToString();
					retVal += str_abi;
					g_pXHost->ReleaseString(str_abi);
					retVal += "\r\n";
				}
			}
			return GetABIString(retVal);
		}
		virtual XObj* Clone() override
		{
			ObjectEvent* pNewEvent = new ObjectEvent(m_name);
			pNewEvent->IncRef();
			return pNewEvent;
		}
		virtual void SetChangeHandler(OnEventHandlerChanged ch) override
		{
			m_changeHandler = ch;
		}
		virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override
		{
			Object::GetBaseScopes(bases);
			bases.push_back(m_APIs.GetMyScope());
		}
		FORCE_INLINE bool WaitOn(int timeout)
		{
			m_statusLock.Lock();
			if (m_fired)
			{
				m_statusLock.Unlock();
				return true;
			}
			XWait* pWait = new XWait();
			m_waits.push_back(pWait);
			m_statusLock.Unlock();
			bool bOK = pWait->Wait(timeout);
			m_statusLock.Lock();
			for (auto it = m_waits.begin(); it != m_waits.end();)
			{
				if (*it == pWait)
				{
					it = m_waits.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
			m_statusLock.Unlock();
			delete pWait;
			return bOK;
		}
		void CleanupUnCallableHandlers()
		{
			//if a handler is remote object but its proxy is invalid, 
			//here is the chance to remove from here to reduce memory usage
			AutoLock autoLock(m_lock);
			for (auto it = m_handlers.begin(); it != m_handlers.end();)
			{
				auto& handleInfo = *it;
				if (handleInfo.ObjectHandler)
				{
					if (handleInfo.ObjectHandler->GetType() == X::ObjType::RemoteObject)
					{
						X::RemoteObject* pRemotObj = dynamic_cast<X::RemoteObject*>(handleInfo.ObjectHandler);
						if (pRemotObj && !pRemotObj->IsValid())
						{
							handleInfo.ObjectHandler->DecRef();
							it = m_handlers.erase(it);
							continue;
						}
					}
				}
				it++;
			}
		}
		virtual bool Call(XRuntime* rt, XObj* pContext, ARGS& params,
			KWARGS& kwParams, X::Value& retValue) override;
		virtual ObjectEvent& operator +=(X::Value& r) override
		{
			CleanupUnCallableHandlers();
			AutoLock autoLock(m_lock);
			if (r.IsObject())
			{
				auto* pObjHandler = dynamic_cast<X::Data::Object*>(r.GetObj());
				pObjHandler->IncRef();
				Add(pObjHandler);
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
		void SetFire()
		{
			m_statusLock.Lock();
			m_fired = true;
			for (auto* pWait : m_waits)
			{
				pWait->Release();
			}
			m_statusLock.Unlock();
		}
		void Fire(X::XRuntime* rt, XObj* pContext,
			ARGS& params, KWARGS& kwargs, bool inMain = false)
		{
			int threadId = -1;
			for (auto& k : kwargs)
			{
				if (k.Match("tid"))
				{
					threadId = (int)k.val.GetLongLong();
					if (!inMain)
					{//for event excuted in main thread,
					 //need to pass tid, not main thread,
					//don't need to pass
						continue;
					}
				}
				Set(k.key, k.val);
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
			SetFire();
			m_lockHandlers.Lock();
			int iCnt = m_handlers.size();
			for (int i = 0; i < iCnt && i < m_handlers.size(); ++i)
			{
				auto& it = m_handlers[i];
				//if (ownerThreadId ==-1 || it.OwnerThreadId == ownerThreadId)
				{
					IncRef();
					if (it.Handler)
					{
						Value retVal; 
						m_lockHandlers.Unlock();
						it.Handler(rt, pContext, params, kwargs, retVal);
						m_lockHandlers.Lock();
					}
					else if (it.ObjectHandler)
					{
						Value retVal;
						m_lockHandlers.Unlock();
						it.ObjectHandler->Call(rt, pContext, params, kwargs, retVal);
						m_lockHandlers.Lock();
					}
					DecRef();
				}
			}
			m_lockHandlers.Unlock();
		}
		FORCE_INLINE X::Value Get(const char* name)
		{
			std::string strName(name);
			return Get(strName);
		}
		FORCE_INLINE X::Value Get(std::string& name)
		{
			X::Value ret;
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				ret = pAttrBag->Get(name);
			}
			return ret;
		}
		FORCE_INLINE void Set(const char* name, X::Value& val)
		{
			std::string strName(name);
			Set(strName, val);
		}
		FORCE_INLINE void Set(std::string& name, X::Value& val)
		{
			auto pAttrBag = GetAttrBag();
			if (pAttrBag)
			{
				pAttrBag->Set(name, val);
			}
		}
		FORCE_INLINE long Add(EventHandler handler)
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
		FORCE_INLINE long Add(X::Data::Object* pObjHandler)
		{
			int cnt = 0;
			int tid = (int)GetThreadID();
			m_lockHandlers.Lock();
			long cookie = ++m_lastCookie;
			EventHandler dummy;
			m_handlers.push_back(HandlerInfo{ dummy,pObjHandler,tid,cookie });
			cnt = (int)m_handlers.size();
			m_lockHandlers.Unlock();
			if (m_changeHandler)
			{
				m_changeHandler(true, cnt);
			}
			return cookie;
		}
		FORCE_INLINE void Remove(long cookie)
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
			ObjectEvent* pEvtObj;
			X::XRuntime* rt;
			X::Value valContext;
			ARGS params;
			KWARGS kwParams;
		};
		struct EventTaskInfo
		{
			EventTask task;
			ARGS params;
		};
		bool m_run = true;
		Locker m_lockEventOnFire;
		std::vector<EventFireInfo> m_eventsOnFire;
		Locker m_lockEventTasks;
		std::vector< EventTaskInfo> m_eventTasks;
		XWait m_wait;
		Locker m_lockEventMap;
		std::unordered_map<std::string, ObjectEvent*> m_eventMap;
	public:
		void Shutdown()
		{
			m_run = false;
			m_wait.Release();
		}
		FORCE_INLINE void FireInMain(ObjectEvent* pEvt, XRuntime* rt, XObj* pContext,
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
		FORCE_INLINE ObjectEvent* Query(const char* name)
		{
			std::string strName(name);
			return Query(strName);
		}
		FORCE_INLINE ObjectEvent* Query(std::string& name)
		{
			ObjectEvent* pEvt = nullptr;
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
		FORCE_INLINE void AddEventTask(EventTask task, ARGS& params)
		{
			m_lockEventTasks.Lock();
			m_eventTasks.push_back(EventTaskInfo{ task,params });
			m_lockEventTasks.Unlock();
			m_wait.Release();
		}
		void Loop()
		{
			while (m_run)
			{
#if (WIN32)
				if (_kbhit())
				{
					char ch = _getch();
					if (ch == 'q' || ch == 'Q')
					{
						break;
					}
				}
#endif	
				m_wait.Wait(1000);
				m_lockEventOnFire.Lock();
				while (m_eventsOnFire.size() > 0)
				{
					auto fireInfo = m_eventsOnFire[0];
					m_eventsOnFire.erase(m_eventsOnFire.begin());
					ObjectEvent* pEvtToRun = fireInfo.pEvtObj;
					m_lockEventOnFire.Unlock();
					pEvtToRun->DecRef();//for m_eventsOnFire
					//todo:
					pEvtToRun->DoFire(fireInfo.rt, fireInfo.valContext.GetObj(),
						fireInfo.params, fireInfo.kwParams);
					m_lockEventOnFire.Lock();
				}
				m_lockEventOnFire.Unlock();
				m_lockEventTasks.Lock();
				while (m_eventTasks.size() > 0)
				{
					auto tskInfo = m_eventTasks[0];
					m_eventTasks.erase(m_eventTasks.begin());
					m_lockEventTasks.Unlock();
					tskInfo.task(tskInfo.params);
					m_lockEventTasks.Lock();
				}
				m_lockEventTasks.Unlock();
			}
		}
		FORCE_INLINE void Fire(X::XRuntime* rt, XObj* pContext,
			std::string& name, ARGS& params, KWARGS& kwargs, bool inMain = false)
		{
			ObjectEvent* pEvt = Query(name);
			if (pEvt)
			{
				pEvt->Fire(rt, pContext, params, kwargs, inMain);
				pEvt->DecRef();
			}
		}
		FORCE_INLINE bool Unregister(const char* name)
		{
			std::string strName(name);
			return Unregister(strName);
		}
		FORCE_INLINE bool Unregister(std::string& name)
		{
			ObjectEvent* pEvt = nullptr;
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
		FORCE_INLINE ObjectEvent* Register(const char* name)
		{
			std::string strName(name);
			return Register(strName);
		}
		ObjectEvent* Register(std::string& name)
		{
			ObjectEvent* pEvt = nullptr;
			m_lockEventMap.Lock();
			auto it = m_eventMap.find(name);
			if (it != m_eventMap.end())
			{
				pEvt = it->second;
			}
			else
			{
				pEvt = new ObjectEvent();
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