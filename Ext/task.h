#ifndef __task_h__
#define __task_h__

#include "gthread.h"
#include "exp.h"

namespace X
{
	class Task :
		public GThread
	{
		AST::Func* m_pFunc = nil;
		AST::Module* m_pModule = nil;
		void* m_pContext = nil;
		std::vector<AST::Value> m_params;
		std::unordered_map<std::string, AST::Value> m_kwParams;
		AST::Value m_retValue;

		// Inherited via GThread
		virtual void run() override;
	public:
		bool Call(AST::Func* pFunc,
			AST::Module* pModule, void* pContext,
			std::vector<AST::Value>& params,
			std::unordered_map<std::string, AST::Value>& kwParams)
		{
			m_pFunc = pFunc;
			m_pModule = pModule;
			m_pContext = pContext;
			m_params = params;
			m_kwParams = kwParams;
			Start();
			//WaitToEnd();
			return true;
		}
	};
}

#endif