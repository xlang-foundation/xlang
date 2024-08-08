#include "glob.h"
#include "event.h"
#include "Locker.h"
#include "object.h"
#include "service_def.h"

namespace X {
	G::G()
	{
		m_lock = (void*)new Locker();
		m_lockRTMap = (void*)new Locker();
		m_lockBreakpointsMap = (void*)new Locker();
		m_lockBreakpointsValid = (void*)new Locker();
	}
	G::~G()
	{
		delete (Locker*)m_lockRTMap;
		delete (Locker*)m_lock;
	}
#if XLANG_ENG_DBG
	void G::ObjBindToStack(XObj* pXObj, AST::StackFrame* pStack)
	{
		Data::Object* pObj = dynamic_cast<Data::Object*>(pXObj);
		Lock();
		auto it = Objects.find(pObj);
		if (it != Objects.end())
		{
			it->second.stacksOwn.push_back(pStack);
		}
		UnLock();
	}
	void G::ObjUnbindToStack(XObj* pXObj, AST::StackFrame* pStack)
	{
		Data::Object* pObj = dynamic_cast<Data::Object*>(pXObj);
		Lock();
		auto it = Objects.find(pObj);
		if (it != Objects.end())
		{
			auto& s_list = it->second.stacksOwn;
			for (auto it2 = s_list.begin(); it2 != s_list.end();)
			{
				if (*it2 == pStack)
				{
					it2 = s_list.erase(it2);
				}
				else
				{
					++it2;
				}
			}
		}
		UnLock();
	}
#endif
	void G::BindRuntimeToThread(XlangRuntime* rt)
	{
		bool bEvent = false;
		//shawn@4/9/2024
		// here we need to use current thread id as key
		//not from the rt itself
		//old code: long long curTId = rt->GetThreadId();
		long long curTId = GetThreadID();
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it == m_rtMap.end())
		{
			m_rtMap.emplace(std::make_pair(curTId,rt));
			bEvent = G::I().GetTrace();
		}
		else
		{
			it->second = rt;
		}
		((Locker*)m_lockRTMap)->Unlock();

		if (bEvent)
			notityThread("ThreadStarted", curTId);
	}
	void G::UnbindRuntimeToThread(XlangRuntime* rt)
	{
		bool bEvent = false;
		//see BindRuntimeToThread
		//old code:long long curTId = rt->GetThreadId();
		long long curTId = GetThreadID();
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it != m_rtMap.end() && it->second == rt)
		{
			m_rtMap.erase(it);
			bEvent = G::I().GetTrace() && !rt->m_bNoDbg;
		}
		((Locker*)m_lockRTMap)->Unlock();

		if (bEvent)
			notityThread("ThreadExited", curTId);
	}
	XlangRuntime* G::MakeThreadRuntime(std::string& name,long long curTId, XlangRuntime* rt)
	{
		bool bEvent = false;
		XlangRuntime* pRet = nullptr;
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it == m_rtMap.end())
		{
			X::XlangRuntime* pRuntime = new X::XlangRuntime();
			if (name == "devops_run.x")
				pRuntime->m_bNoDbg = true;

			pRuntime->SetName(name);
			if (rt)
			{
				pRuntime->MirrorStacksFrom(rt);
			}
			m_rtMap.emplace(std::make_pair(curTId, pRuntime));
			pRet = pRuntime;
			bEvent = G::I().GetTrace() && !pRuntime->m_bNoDbg;
		}
		else
		{
			pRet = it->second;
		}
		((Locker*)m_lockRTMap)->Unlock();

		if (bEvent)
			notityThread("ThreadStarted", curTId);
		
		return pRet;
	}

	void G::notityThread(const char* strType, int tid)
	{
		
		KWARGS kwParams;
		X::Value valAction("notify");
		kwParams.Add("action", valAction);
		const int online_len = 1000;
		char strBuf[online_len];
		SPRINTF(strBuf, online_len, "[{\"%s\":%d}]", strType, tid);
		X::Value valParam(strBuf);
		kwParams.Add("param", valParam);
		std::cout << strType << " threadId: " << tid << std::endl;
		std::string evtName("devops.dbg");
		ARGS params(0);
		X::EventSystem::I().Fire(nullptr, nullptr, evtName, params, kwParams);
	}

	void G::Lock()
	{
		((Locker*)m_lock)->Lock();
	}
	void G::UnLock()
	{
		((Locker*)m_lock)->Unlock();
	}
}