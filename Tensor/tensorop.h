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
		public:
			TensorOperator():XTensor(0),Object(),Tensor()
			{
			}
			~TensorOperator()
			{

			}
			void SetOpAction(X::Value& action)
			{
				m_opAction = action;
			}
		};
	}
}