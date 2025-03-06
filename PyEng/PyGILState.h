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