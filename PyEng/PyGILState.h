#pragma once

#include "PyFunc.h"

class MGil
{
	int m_gstate = -1;
public:
	inline MGil(bool autoLock = true)
	{
		if (autoLock)
		{
			m_gstate = (int)PyGILState_Ensure();
		}
	}
	inline ~MGil()
	{
		if (m_gstate != -1)
		{
			PyGILState_Release((PyGILState_STATE)m_gstate);
		}
	}
	inline void Lock()
	{
		m_gstate = (int)PyGILState_Ensure();
	}
	inline void Unlock()
	{
		if (m_gstate != -1)
		{
			PyGILState_Release((PyGILState_STATE)m_gstate);
			m_gstate = -1;
		}
	}
};