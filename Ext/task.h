#ifndef __task_h__
#define __task_h__

#include "gthread.h"
#include "exp.h"
#include "runtime.h"

namespace X
{
	class Runtime;
	class Task :
		public GThread
	{
		AST::Func* m_pFunc = nil;
		Runtime* m_rt = nil;
		void* m_pContext = nil;
		ARGS m_params;
		KWARGS m_kwParams;
		AST::Value m_retValue;

		// Inherited via GThread
		virtual void run() override;
	public:
		bool Call(AST::Func* pFunc,
			Runtime* rt, void* pContext,
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