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

#include "object.h"
#include "tensor_expression.h"
#include "code_generator.h"
#include <unordered_map>
#include <string>
#include <algorithm>
#include <numeric>
#include <cstddef>
#include <cstring> 

namespace X
{
	namespace Data
	{
		class GraphBuildContext
		{
			//match with Tensor_Operator in tensor.h
			std::vector<Tensor_OperatorHandler> m_Handlers;
			std::unordered_map<TensorExpression*, bool> m_BeCalledMap;
		public:
			GraphBuildContext()
			{
				Tensor_OperatorHandler dummy;
				for (int i = 0; i < (int)Tensor_Operator::Count; i++)
				{
					m_Handlers.push_back(dummy);
				}
			}
			Tensor_OperatorHandler QueryHandler(int idx)
			{
				return m_Handlers[idx];
			}
			void SetHandler(int idx, Tensor_OperatorHandler handler)
			{
				m_Handlers[idx] = handler;
			}
			bool IsCalledBuild(TensorExpression* exp)
			{
				auto it = m_BeCalledMap.find(exp);
				if (it != m_BeCalledMap.end())
				{
					return it->second;
				}
				else
				{
					return false;
				}
			}
			void SetBuildCalled(TensorExpression* exp)
			{
				m_BeCalledMap.emplace(std::make_pair(exp, true));
			}
		};
		std::string GetNameWithOp(Tensor_Operator op);
		class TensorGraph :
			virtual public XTensorGraph,
			virtual public Object
		{
			enum class GraphBuildAction
			{		
				None,
				MeetBinaryOp,
			};
			CodeGenerator m_gen;
			int m_LastInstructionId = 0;
			void BuildGraph(void* pBuildContext,
				XObj* pContext, TensorExpression* pTensor, GraphBuildAction& retAction);

			X::Value m_runner;//which impl. ops for tensor
			std::vector<TensorRunItem> m_runItems;
			std::unordered_map<unsigned long long, FlowBlock> m_flowBlocks;  // Control flow blocks

			// Cache for tensors created during graph building
			std::unordered_map<unsigned long long, X::Value> m_TensorCache;
			Tensor_OperatorHandler QueryRegisteredOpHandler(void* pBuildContext,
				XObj* pPackage, int opIndex);

			unsigned long long GetOrCreateFlowBlock(X::AST::If* pIfStmt, unsigned long long parentFlowId = 0, int parentBranchId = -1);

			// Helper function to swap two TensorRunItem objects using memcpy.
			inline void memcpySwap(TensorRunItem& a, TensorRunItem& b) {
				unsigned char temp[sizeof(TensorRunItem)];
				memcpy(temp, &a, sizeof(TensorRunItem));
				memcpy(&a, &b, sizeof(TensorRunItem));
				memcpy(&b, temp, sizeof(TensorRunItem));
			}

			void SortRunItems() {
				// Helper lambda: treat branch == -1 as a very high value so that it sorts last.
				auto effectiveBranch = [](int branch) -> int {
					return (branch == -1) ? 1000000 : branch;
					};

				size_t n = m_runItems.size();
				// Loop over all items.
				for (size_t i = 0; i < n; ++i) {
					// Skip items with flowId 0 (keep their order unchanged)
					if (m_runItems[i].flowId == 0)
						continue;
					// For items with nonzero flowId, check all later items.
					for (size_t j = i + 1; j < n; ++j) {
						// Only consider items with the same flowId.
						if (m_runItems[j].flowId == m_runItems[i].flowId) {
							// If a later item has an effective branch lower than the current one,
							// swap them using memcpySwap.
							if (effectiveBranch(m_runItems[j].branchId) < effectiveBranch(m_runItems[i].branchId)) {
								memcpySwap(m_runItems[i], m_runItems[j]);
							}
						}
					}
				}
			}


		public:
			static bool IsInIfStatement(X::AST::Expression* expr, X::AST::If** ppIfStmt, int* pBranchId);

		public:
			static void Init();
			static void cleanup();
			TensorGraph() :XTensorGraph(0), XObj(), Object(), m_gen(this)
			{
				m_t = ObjType::TensorGraph;
			}
			FORCE_INLINE X::Value GetBlockCondition(unsigned long long flowId,int branchId)
			{
				return m_flowBlocks[flowId].branches[branchId].condition;
			}
			virtual void Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams) override;
			virtual void PutTensorIntoCache(X::Value& vTensor) override;
			virtual void RemoveTensorFromCache(X::Value& vTensor) override;
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			bool Run(X::ARGS& params, X::KWARGS& kwParams);
			virtual const char* ToString(bool WithFormat = false) override;
		};
	}
}