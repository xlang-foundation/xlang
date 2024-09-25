/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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
			FORCE_INLINE X::Value& GetLeftValue() { return m_leftVal; }
			FORCE_INLINE X::Value& GetRightValue() { return m_rightVal; }
			FORCE_INLINE Tensor_Operator GetOp() { return m_op; }
		};
	}
}

