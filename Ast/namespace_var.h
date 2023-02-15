#pragma once
#include "block.h"
#include <assert.h> 
#include "scope.h"

namespace X
{
	namespace AST
	{
		class NamespaceVar :
			virtual public Block
		{
			int m_Index = -1;
		public:
			NamespaceVar() :
				Block(), UnaryOp(), Operator()
			{
				m_type = ObType::NamespaceVar;
			}
			NamespaceVar(short op) :
				Block(), UnaryOp(), Operator(op)
			{
				m_type = ObType::NamespaceVar;
			}
			~NamespaceVar()
			{

			}
			inline virtual bool Set(XlangRuntime* rt, XObj* pContext, Value& v) override
			{
				if (m_Index == -1)
				{
					ScopeLayout();
					assert(m_Index != -1 && m_scope != nullptr);
				}
				return m_scope->Set(rt, pContext, m_Index, v);
			}
			virtual void Add(Expression* item) override;
			virtual void ScopeLayout() override;
			virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands)
			{
				if (!operands.empty()
					&& operands.top()->GetTokenIndex() > m_tokenIndex)
				{
					R = operands.top();
				}
				operands.push(this);
				return true;
			}
			virtual bool Exec(XlangRuntime* rt, 
				ExecAction& action, XObj* pContext, 
				Value& v, LValue* lValue = nullptr) override;
		};
	}
}