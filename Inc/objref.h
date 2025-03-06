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

namespace X
{
	class ObjRef
	{
		int m_ref = 0;
	protected:
		FORCE_INLINE virtual int AddRef()
		{
			return ++m_ref;
		}
		FORCE_INLINE virtual int Release()
		{
			return --m_ref;
		}
	public:
		FORCE_INLINE int Ref() { return m_ref; }
		ObjRef() {}
		//line below, need to use virtual
		//for virtual inheritance from derived class
		virtual ~ObjRef() {}
	};
}