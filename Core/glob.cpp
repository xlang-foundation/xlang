#include "glob.h"
#include "Locker.h"

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