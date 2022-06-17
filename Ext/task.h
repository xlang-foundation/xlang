#ifndef __task_h__
#define __task_h__

#include "gthread.h"
#include "exp.h"

namespace X
{
	class Runtime;
	class Task :
		public GThread
	{
		AST::Func* m_pFunc = nil;
		Runtime* m_rt = nil;
		void* m_pContext = nil;
		std::vector<AST::Value> m_params;
		std::unordered_map<std::string, AST::Value> m_kwParams;
		AST::Value m_retValue;

		// Inherited via GThread
		virtual void run() override;
	public:
		bool Call(AST::Func* pFunc,
			Runtime* rt, void* pContext,
			std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams)
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