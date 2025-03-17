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
#include "xlang.h"

namespace X
{
	namespace Data
	{
		enum class Tensor_Operator
		{
			None,
			Header,Trailer, BranchBegin, BranchEnd,
			Add, Minus, Mul, Div,
			Count
		};

		//Branch within a flow block (if, elif, or else)
		struct FlowBranch
		{
			int id;                     // Branch ID (0=if, 1=elif1, 2=elif2, ..., -1=else)
			X::Value condition;         // Condition expression for this branch
		};

		// Flow block (entire if/elif/else structure)
		struct FlowBlock
		{
			int id;                     // Unique ID for this flow block
			std::vector<FlowBranch> branches;  // All branches in this block
			int parentId = -1;          // Parent flow block ID (for nesting)
			int parentBranchId = -1;    // Branch ID in parent that contains this block
		};

		struct TensorRunItem
		{
			std::string name;
			Tensor_OperatorHandler handler;
			X::ARGS inputs;
			X::Value output;

			// Control flow tracking
			unsigned long long flowId = 0;       // ID of the flow block this item belongs to
			int branchId = -1;     // Branch ID within the flow block (0=if, 1=elif1, -1=else)

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

			FORCE_INLINE void SetLeftVal(X::Value& val)
			{
				m_leftVal = val;
			}
			FORCE_INLINE void SetRightVal(X::Value& val, Tensor_Operator op)
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

