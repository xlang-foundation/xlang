#ifndef __task_h__
#define __task_h__

#include "gthread.h"
#include "exp.h"
#include "runtime.h"
#include "xlang.h"

namespace X
{
	class Runtime;
	class Task :
		public GThread
	{
		AST::Func* m_pFunc = nil;
		Runtime* m_rt = nil;
		XObj* m_pContext = nil;
		ARGS m_params;
		KWARGS m_kwParams;
		X::Value m_retValue;

		// Inherited via GThread
		virtual void run() override;
	public:
		bool Call(AST::Func* pFunc,
			Runtime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams)
		{
			m_pFunc = pFunc;
			m_pContext = pContext;
			m_params = params;
			m_kwParams = kwParams;

			//copy stacks into new thread
			X::Runtime* pRuntime = new X::Runtime();
			pRuntime->MirrorStacksFrom(rt);
			m_rt = pRuntime;
			Start();
			//WaitToEnd();
			return true;
		}
	};
}

#endif