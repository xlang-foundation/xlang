#include "glob.h"
#include "Locker.h"
#include "object.h"

namespace X {
	G::G()
	{
		m_lock = (void*)new Locker();
		m_lockRTMap = (void*)new Locker();
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
		long long curTId = rt->GetThreadId();
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it == m_rtMap.end())
		{
			m_rtMap.emplace(std::make_pair(curTId,rt));
		}
		else
		{
			it->second = rt;
		}
		((Locker*)m_lockRTMap)->Unlock();
	}
	void G::UnbindRuntimeToThread(XlangRuntime* rt)
	{
		long long curTId = rt->GetThreadId();
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it != m_rtMap.end())
		{
			m_rtMap.erase(it);
		}
		((Locker*)m_lockRTMap)->Unlock();
	}
	XlangRuntime* G::MakeThreadRuntime(long long curTId, XlangRuntime* rt)
	{
		XlangRuntime* pRet = nullptr;
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it == m_rtMap.end())
		{
			X::XlangRuntime* pRuntime = new X::XlangRuntime();
			if (rt)
			{
				pRuntime->MirrorStacksFrom(rt);
			}
			m_rtMap.emplace(std::make_pair(curTId, pRuntime));
			pRet = pRuntime;
		}
		else
		{
			pRet = it->second;
		}
		((Locker*)m_lockRTMap)->Unlock();
		return pRet;
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