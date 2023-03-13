#pragma once
#include "tensor.h"

namespace X
{
	namespace Data
	{
		//base class for all tensor's ops
		class TensorOperator :
			virtual public Tensor
		{
			X::Value m_opAction;//like a function to work with tensor
			Tensor_OperatorHandler m_opHandler;
			bool m_isUnaryOp = false;
		public:
			TensorOperator(Tensor_OperatorHandler op, bool isUnaryOp):XTensor(0),Object(),Tensor()
			{
				m_opHandler = op;
				m_isUnaryOp = isUnaryOp;
			}
			~TensorOperator()
			{

			}
			inline bool IsUnaryOp() { return m_isUnaryOp; }
			inline Tensor_OperatorHandler GetOpHandler()
			{
				return m_opHandler;
			}
			//for Tensor Operator,don't need to create a new instance
			//for exampl t1*T.mul()*t2, when calc T.mul()*t2, T.mul() already a new instance
			//can be used to accept t2
			//so overide back to call Object's impl.
			virtual XObj* Clone() override
			{
				return Object::Clone();
			}
			void SetOpAction(X::Value& action)
			{
				m_opAction = action;
			}
		};
	}
}