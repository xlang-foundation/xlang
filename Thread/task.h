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

#ifndef __task_h__
#define __task_h__

#include "exp.h"
#include "runtime.h"
#include "xlang.h"

namespace X
{
	class XlangRuntime;
	class Task
	{
		X::Value m_taskPool;
		X::Value m_valFunc;
		X::Value m_future;
		XlangRuntime* m_rt = nil;
		X::Value m_context;
		ARGS m_params;
		KWARGS m_kwParams;
		X::Value m_retValue;

	public:
		Task() :m_params(0)
		{

		}
		void SetTaskPool(X::Value& pool)
		{
			m_taskPool = pool;
		}
		void SetFuture(X::Value& f)
		{
			m_future = f;
		}
		void run();
		bool Call(X::Value& valFunc,
			XlangRuntime* rt, XObj* pContext,
			ARGS& params,KWARGS& kwParams);
	};
}

#endif