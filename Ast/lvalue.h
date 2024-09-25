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

#include "value.h"

namespace X
{
	class XObj;
	class LValue
	{
		Value val;//for some cases, the value is on stack, 
		//can't keep when out of scope
		//so add here,but it is enable when valptr is null
		Value* valptr = nullptr;
		bool releaseValPtr = false;
		XObj* context = nullptr;
	public:
		FORCE_INLINE void SetReleaseFlag(bool b)
		{
			releaseValPtr = b;
		}
		FORCE_INLINE LValue()
		{
			valptr = nullptr;
		}
		FORCE_INLINE LValue(Value& v)
		{
			val = v;
			//todo: CHANGE TO NULL, because v maybe in stack, 
			//as temp var
			valptr = nullptr;// &val;
		}
		FORCE_INLINE LValue(Value* pVal)
		{
			valptr = pVal;
		}
		FORCE_INLINE ~LValue()
		{
			if (releaseValPtr && valptr)
			{
				delete valptr;
			}
		}
		FORCE_INLINE void SetContext(XObj* p) { context = p; }
		FORCE_INLINE XObj* GetContext() { return context; }
		FORCE_INLINE operator bool()
		{
			return (valptr != 0) || val.IsValid();
		}
		FORCE_INLINE Value& operator *() const
		{
			if (valptr)
			{
				return *valptr;
			}
			else
			{
				return (Value&)val;
			}
		}
		FORCE_INLINE Value* operator ->() const
		{
			return valptr;
		}
	};
}