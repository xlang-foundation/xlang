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
#include "utility.h"
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
		long long m_enqueueTime;
		long long m_startRunTime;
		long long m_endRunTime;
	public:
		Task() :m_params(0),m_startRunTime(0), m_endRunTime(0)
		{
			m_enqueueTime = getCurMicroTimeStamp();
		}
		void Cancel();
		void Cancelled();
		void SetTaskPool(X::Value& pool)
		{
			m_taskPool = pool;
		}
		void SetFuture(X::Value& f)
		{
			m_future = f;
		}
		inline void SetEnqueueTime(long long t)
		{
			m_enqueueTime = t;
		}
		inline void SetStartRunTime(long long t)
		{
			m_startRunTime = t;
		}
		inline void SetEndRunTime(long long t)
		{
			m_endRunTime = t;
		}
		inline long long GetEnqueueTime()
		{
			return m_enqueueTime;
		}
		inline long long GetStartRunTime()
		{
			return m_startRunTime;
		}
		inline long long GetEndRunTime()
		{
			return m_endRunTime;
		}
		void run();
		bool Call(X::Value& valFunc,
			XlangRuntime* rt, XObj* pContext,
			ARGS& params,KWARGS& kwParams);
	};
}

#endif