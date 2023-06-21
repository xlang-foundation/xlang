#pragma once
#include "exp.h"
#include "op.h"


namespace X
{
	namespace AST
	{
		class AwaitOp :
			virtual public UnaryOp
		{
		public:
			AwaitOp() :
				Operator(),
				UnaryOp()
			{
				m_type = ObType::AwaitOp;
			}
			AwaitOp(short op) :
				Operator(op),
				UnaryOp(op)
			{
				m_type = ObType::AwaitOp;
			}
			virtual void ScopeLayout() override
			{

			}
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override;
		};
	}
}