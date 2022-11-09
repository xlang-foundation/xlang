#pragma once
#include "exp.h"
#include "op.h"
namespace X
{
	namespace AST
	{
		/*
			for other domain-specific language support
			add % at the begin to start a new line also can use line feed \ to crosss multiple
			lines
		*/
		class FeedOp :
			virtual public Operator
		{
		protected:
			char* m_s = nil;
			int m_size = 0;
		public:
			FeedOp(char* s, int size)
			{
				m_type = ObType::FeedOp;
				m_s = s;
				m_size = size;
			}
			inline virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands)
			{
				operands.push(this);
				return true;
			}
			virtual void ScopeLayout() override
			{

			}
			virtual bool Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override
			{
				std::string info;
				RunStringExpWithFormat(rt, pContext, m_s+1, m_size-1, info);
				std::cout << info << std::endl;
				return true;
			}
		};
	}
}