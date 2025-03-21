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

#include "tensor_graph.h"
#include "function.h"
#include "tensor.h"
#include "tensor_expression.h"
#include "tensorop.h"
#include <iostream>
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<1> _tensorGraphScope;
		void TensorGraph::Init()
		{
			_tensorGraphScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					TensorGraph* pGraph = dynamic_cast<TensorGraph*>(pContext);
					retValue = X::Value(pGraph->Run(params, kwParams));
					return true;
				};
				_tensorGraphScope.AddFunc("run", "graph.run()", f);
			}
			_tensorGraphScope.Close();
		}

		void TensorGraph::cleanup()
		{
			_tensorGraphScope.Clean();
		}
		void TensorGraph::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_tensorGraphScope.GetMyScope());
		}

	
		Tensor_OperatorHandler TensorGraph::QueryRegisteredOpHandler(void* pBuildContext,
			XObj* pPackage, int opIndex)
		{
			GraphBuildContext* pGraphBuildContext = (GraphBuildContext*)pBuildContext;
			Tensor_OperatorHandler handler = pGraphBuildContext->QueryHandler(opIndex);
			if (handler)
			{
				return handler;
			}
			auto* pObjPackage = dynamic_cast<X::Data::Object*>(pPackage);
			AST::Scope* pScope = pObjPackage->GetMyScope();
			if (pScope)
			{
				std::string strName;
				switch ((Tensor_Operator)opIndex)
				{
				case X::Data::Tensor_Operator::None:
					break;
				case X::Data::Tensor_Operator::Header:
					strName = "header";
					break;
				case X::Data::Tensor_Operator::Trailer:
					strName = "trailer";
					break;
				case X::Data::Tensor_Operator::BranchBegin:
					strName = "branchBegin";
					break;
				case X::Data::Tensor_Operator::BranchEnd:
					strName = "branchEnd";
					break;
				case X::Data::Tensor_Operator::Add:
					strName = "add";
					break;
				case X::Data::Tensor_Operator::Minus:
					strName = "minus";
					break;
				case X::Data::Tensor_Operator::Mul:
					strName = "mul";
					break;
				case X::Data::Tensor_Operator::Div:
					strName = "div";
					break;
				default:
					break;
				}
				SCOPE_FAST_CALL_AddOrGet0(idx,pScope,strName, true);
				if (idx >= 0)
				{
					X::Value valFunc;
					if (pPackage->GetIndexValue(idx, valFunc))
					{
						if (valFunc.IsObject())
						{
							XObj* pObjHandler = valFunc.GetObj();
							X::ARGS args(0);
							X::KWARGS kwargs;
							X::Value valTensorOp;
							pObjHandler->Call(nullptr, pPackage, args, kwargs, valTensorOp);
							if (valTensorOp.IsObject())
							{
								TensorOperator* pTensorOp = dynamic_cast<TensorOperator*>(valTensorOp.GetObj());
								if (pTensorOp)
								{
									handler = pTensorOp->GetOpHandler();
									pGraphBuildContext->SetHandler(opIndex, handler);
								}
							}
						}
					}
				}
			}
			return handler;
		}
		std::string GetNameWithOp(Tensor_Operator op)
		{
			std::string strName;
			switch (op)
			{
			case X::Data::Tensor_Operator::None:
				break;
			case X::Data::Tensor_Operator::Add:
				strName = "add";
				break;
			case X::Data::Tensor_Operator::Minus:
				strName = "minus";
				break;
			case X::Data::Tensor_Operator::Mul:
				strName = "mul";
				break;
			case X::Data::Tensor_Operator::Div:
				strName = "div";
				break;
			default:
				break;
			}
			return strName;
		}

		void TensorGraph::Create(XObj* pContext, X::ARGS& params, X::KWARGS& kwParams)
		{
			m_runner = X::Value(pContext);

			GraphBuildContext buildContext;
			m_gen.setHeaderHandler(
				QueryRegisteredOpHandler(&buildContext, pContext,
					(int)Tensor_Operator::Header));
			m_gen.setTrailerHandler(
				QueryRegisteredOpHandler(&buildContext, pContext,
					(int)Tensor_Operator::Trailer));
			m_gen.setBranchBeginHandler(
				QueryRegisteredOpHandler(&buildContext, pContext,
					(int)Tensor_Operator::BranchBegin));
			m_gen.setBranchEndHandler(
				QueryRegisteredOpHandler(&buildContext, pContext,
					(int)Tensor_Operator::BranchEnd));

			//check tensor's m_op, if the tensor package(pContext) has
			//this operator impl., replace with that
			for (auto& p : params)
			{
				if (!p.IsObject() || p.GetObj()->GetType() != ObjType::TensorExpression)
				{
					continue;
				}
				TensorExpression* tensorExp = dynamic_cast<TensorExpression*>(p.GetObj());
				GraphBuildAction action = GraphBuildAction::None;
				BuildGraph(&buildContext, pContext, tensorExp, action);
			}
		}
		void TensorGraph::PutTensorIntoCache(X::Value& vTensor)
		{
			// Store tensor in cache using its ID as key
			m_TensorCache.emplace(vTensor.GetObj()->GetID(), vTensor);
		}
		void TensorGraph::RemoveTensorFromCache(X::Value& vTensor)
		{
			// Remove tensor from cache using its ID
			m_TensorCache.erase(vTensor.GetObj()->GetID());	
		}
		// Add this enhanced ToString method to display control flow structure

		const char* TensorGraph::ToString(bool WithFormat)
		{
			std::stringstream outStr;

			// First part: Show regular run items
			outStr << "Run Items:\n";
			for (size_t i = 0; i < m_runItems.size(); i++)
			{
				auto& item = m_runItems[i];
				std::string lineOut = item.name;

				// Add flow information if present
				if (item.flowId >= 0) {
					lineOut += " [flow=" + std::to_string(item.flowId) +
						", branch=" + std::to_string(item.branchId) + "]";
				}

				// Add inputs
				for (auto& in : item.inputs)
				{
					std::string inputName;
					if (in.IsObject())
					{
						if (in.GetObj()->GetType() == ObjType::Tensor)
						{
							Tensor* pTensor = dynamic_cast<Tensor*>(in.GetObj());
							if (pTensor)
							{
								inputName = pTensor->GetTensorName();
							}
						}
						else if (in.GetObj()->GetType() == ObjType::TensorExpression)
						{
							TensorExpression* pTensorExp = dynamic_cast<TensorExpression*>(in.GetObj());
							if (pTensorExp)
							{
								inputName = pTensorExp->GetTensorName();
							}
						}
					}
					else
					{
						inputName = in.ToString(); // For scalar 
					}
					lineOut += " " + inputName;
				}

				// Add output
				if (item.output.IsObject())
				{
					std::string outputName;
					if (item.output.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pTensor = dynamic_cast<Tensor*>(item.output.GetObj());
						if (pTensor)
						{
							outputName = pTensor->GetTensorName();
						}
					}
					else if (item.output.GetObj()->GetType() == ObjType::TensorExpression)
					{
						TensorExpression* pTensorExp = dynamic_cast<TensorExpression*>(item.output.GetObj());
						if (pTensorExp)
						{
							outputName = pTensorExp->GetTensorName();
						}
					}
					lineOut += " -> " + outputName;
				}
				outStr << i << ": " << lineOut << "\n";
			}

			// Second part: Show flow blocks structure
			if (!m_flowBlocks.empty()) {
				outStr << "\nControl Flow Structure:\n";

				// First show top-level blocks
				for (const auto& pair : m_flowBlocks) {
					const FlowBlock& block = pair.second;
					if (block.parentId < 0) {
						outStr << "Flow Block " << block.id << ":\n";

						// Show branches
						for (const FlowBranch& branch : block.branches) {
							if (branch.id == 0) {
								outStr << "  if: ";
							}
							else if (branch.id > 0) {
								outStr << "  elif " << branch.id << ": ";
							}
							else {
								outStr << "  else: ";
							}

							// Count operations in this branch
							int opCount = 0;
							for (const auto& item : m_runItems) {
								if (item.flowId == block.id && item.branchId == branch.id) {
									opCount++;
								}
							}
							outStr << opCount << " operations\n";

							// Show operations in this branch
							for (size_t i = 0; i < m_runItems.size(); i++) {
								const auto& item = m_runItems[i];
								if (item.flowId == block.id && item.branchId == branch.id) {
									outStr << "    " << i << ": " << item.name << "\n";
								}
							}

							// Show nested blocks in this branch
							bool hasNested = false;
							for (const auto& nestedPair : m_flowBlocks) {
								const FlowBlock& nestedBlock = nestedPair.second;
								if (nestedBlock.parentId == block.id && nestedBlock.parentBranchId == branch.id) {
									if (!hasNested) {
										outStr << "    Nested blocks:\n";
										hasNested = true;
									}
									outStr << "      Flow Block " << nestedBlock.id << "\n";
								}
							}
						}

						outStr << "\n";
					}
				}
			}
			std::string str = outStr.str();
			return GetABIString(str);
		}
	}
}