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