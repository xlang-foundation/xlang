#pragma once
#include "exp.h"
#include "op.h"


namespace X
{
	namespace AST
	{
		class AwaitOp :
			public UnaryOp
		{
		public:
			AwaitOp() :
				UnaryOp()
			{
				m_type = ObType::AwaitOp;
			}
			AwaitOp(short op) :
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