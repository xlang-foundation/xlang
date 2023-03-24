#pragma once
#include "tensor.h"
#include "ops_mgt.h"

namespace X
{
	namespace Data
	{
		enum class Tensor_Operator
		{
			None, Add, Minus, Mul, Div,
			Count
		};
		class TensorExpression :
			virtual public Tensor
		{
			X::Value m_leftVal;
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
		public:
			TensorExpression() :Tensor(),XTensor(0),XObj(), Object()
			{
				m_t = ObjType::TensorExpression;
			}
			void SetLeftVal(X::Value& val)
			{
				m_leftVal = val;
			}
			void SetRightVal(X::Value& val, Tensor_Operator op)
			{
				m_rightVal = val;
				m_op = op;
			}
			inline X::Value& GetLeftValue() { return m_leftVal; }
			inline X::Value& GetRightValue() { return m_rightVal; }
			inline Tensor_Operator GetOp() { return m_op; }
		};
	}
}

