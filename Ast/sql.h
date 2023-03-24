#pragma once
#include "exp.h"
#include "op.h"

/*
add directly SQL Statement support in xlang
*/

#if ADD_SQL
namespace X
{
	namespace AST
	{
		class Select :
			virtual public UnaryOp
		{
		public:
			Select() :Operator(), UnaryOp()
			{
				m_type = ObType::Select;
			}
			Select(short op) :
				Operator(op),
				UnaryOp(op)
			{
				m_type = ObType::Select;
			}
			~Select()
			{
			}
			virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands, int LeftTokenIndex) override
			{
				return true;
			}
		};
	}
}
#endif
