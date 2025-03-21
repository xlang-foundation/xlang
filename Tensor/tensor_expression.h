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
	namespace AST
	{
		class If;
	}
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
			unsigned long long id;                     // Unique ID for this flow block
			std::vector<FlowBranch> branches;  // All branches in this block
			unsigned long long parentId = 0;          // Parent flow block ID (for nesting)
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
		struct FlowBranchInfo
		{
			X::AST::Expression* line;// this TensorExpression's current line
			X::AST::If* ifEntry;
			int branchId;
		};
		class TensorExpression :
			virtual public Tensor
		{
			X::Value m_leftVal;
			X::Value m_rightVal; //maybe a Tensor or TensorOperator
			Tensor_Operator m_op = Tensor_Operator::None;
			FlowBranchInfo m_branch = { nullptr,nullptr,0 }; //used to hold if/elif/else branches
			std::vector<X::Value> m_subs;//for branches
		public:
			virtual void SetCurrentLine(X::AST::Expression* line) override;

			TensorExpression() :Tensor(),XTensor(0),XObj(), Object()
			{
				m_t = ObjType::TensorExpression;
			}
			FORCE_INLINE bool HasBranch() const { return m_branch.ifEntry != nullptr; } 
			FORCE_INLINE virtual bool SupportAssign() { return true; }
			FORCE_INLINE virtual bool Assign(const X::Value& val) override
			{
				if (val.IsObject() && val.GetObj()->GetType() == X::ObjType::TensorExpression)
				{
					TensorExpression* pSubCandidate = dynamic_cast<TensorExpression*>(val.GetObj());
					if (pSubCandidate && pSubCandidate->HasBranch())
					{
						bool hasSameBranch = false;
						size_t indexToReplace = 0;

						// First, find the element with matching branch
						for (size_t i = 0; i < m_subs.size(); ++i)
						{
							TensorExpression* pSub = dynamic_cast<TensorExpression*>(m_subs[i].GetObj());
							if (pSub)
							{
								if (pSub->m_branch.ifEntry == pSubCandidate->m_branch.ifEntry
									&& pSub->m_branch.branchId == pSubCandidate->m_branch.branchId)
								{
									indexToReplace = i;
									hasSameBranch = true;
									break;
								}
							}
						}
						//we use this way to avoid direct assign will call this function again
						if (hasSameBranch)
						{
							// Replace without assignment by removing and inserting
							m_subs.erase(m_subs.begin() + indexToReplace);
							m_subs.insert(m_subs.begin() + indexToReplace, val);
							return true;
						}
						m_subs.push_back(val); // add to subs
						return true;
					}
				}
				return false; // not support assign
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
			FORCE_INLINE const std::vector<X::Value>& GetSubExpressions() const {
				return m_subs;
			}
			// Get the branch information
			FORCE_INLINE const FlowBranchInfo& GetBranchInfo() const {
				return m_branch;
			}
			FORCE_INLINE X::Value& GetLeftValue() { return m_leftVal; }
			FORCE_INLINE X::Value& GetRightValue() { return m_rightVal; }
			FORCE_INLINE Tensor_Operator GetOp() { return m_op; }
		};
	}
}

