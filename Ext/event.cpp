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

#include "event.h"
namespace X
{
	bool ObjectEvent::Call(XRuntime* rt, XObj* pContext,
		ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		auto* findSetValue = kwParams.find("SetValue");
		if (findSetValue != nullptr && params.size()>0)
		{
			*this += params[0];
			return true;
		}

		DoFire(rt, pContext, params, kwParams);
		retValue = Value(true);
		return true;
	}
	void ObjectEvent::FireInMain(X::XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwargs)
	{
		SetFire();
		EventSystem::I().FireInMain(this,rt,pContext,params,kwargs);
	}
}