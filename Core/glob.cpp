#include "glob.h"
#include "Locker.h"

namespace X {
	G::G()
	{
		m_lock = (void*)new Locker();
	}
	G::~G()
	{
		delete (Locker*)m_lock;
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