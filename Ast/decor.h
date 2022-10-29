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
			bool GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams);
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
			~Decorator()
			{

			}
			inline Expression* Client() { return m_client; }
			inline void SetClient(Expression* e) { m_client = e; }
			virtual bool Run(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}