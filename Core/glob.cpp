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
	
	Runtime* G::MakeThreadRuntime(long long curTId, Runtime* rt)
	{
		Runtime* pRet = nullptr;
		((Locker*)m_lockRTMap)->Lock();
		auto it = m_rtMap.find(curTId);
		if (it == m_rtMap.end())
		{
			X::Runtime* pRuntime = new X::Runtime();
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