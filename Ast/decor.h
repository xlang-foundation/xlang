#pragma once
#include "op.h"

namespace X
{
	namespace AST
	{
		class Decorator :
			virtual public UnaryOp
		{
			Expression* m_client = nullptr;
		public:
			Decorator() :
				Operator(),
				UnaryOp()
			{
				m_type = ObType::Decor;
			}
			Decorator(short op) :
				Operator(op),
				UnaryOp(op)
			{
				m_type = ObType::Decor;
			}
			inline Expression* Client() { return m_client; }
			inline void SetClient(Expression* e) { m_client = e; }
			virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}