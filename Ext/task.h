#ifndef __task_h__
#define __task_h__

#include "gthread.h"
#include "exp.h"
#include "runtime.h"
#include "xlang.h"

namespace X
{
	class XlangRuntime;
	class Task :
		public GThread
	{
		AST::Func* m_pFunc = nil;
		XlangRuntime* m_rt = nil;
		XObj* m_pContext = nil;
		ARGS m_params;
		KWARGS m_kwParams;
		X::Value m_retValue;

		// Inherited via GThread
		virtual void run() override;
	public:
		bool Call(AST::Func* pFunc,
			XlangRuntime* rt, XObj* pContext,
			ARGS& params,
			KWARGS& kwParams)
		{
			m_pFunc = pFunc;
			m_pContext = pContext;
			m_params = params;
			m_kwParams = kwParams;

			//copy stacks into new thread
			X::XlangRuntime* pRuntime = new X::XlangRuntime();
			pRuntime->MirrorStacksFrom(rt);
			m_rt = pRuntime;
			Start();
			//WaitToEnd();
			return true;
		}
	};
}

#endif