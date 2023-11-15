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
			public Operator
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
				std::stack<AST::Expression*>& operands, int LeftTokenIndex)
			{
				operands.push(this);
				return true;
			}
			virtual void ScopeLayout() override
			{

			}
			virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}