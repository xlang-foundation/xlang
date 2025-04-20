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
        void TensorGraph::BuildGraph(void* pBuildContext,
            XObj* pContext, TensorExpression* pTensor, GraphBuildAction& retAction)
        {
            GraphBuildContext* pGraphBuildContext = (GraphBuildContext*)pBuildContext;
            GraphBuildAction thisLevel_Action = GraphBuildAction::None;
            Tensor_Operator op = pTensor->GetOp();

            // First, check if this tensor has an operation to process
            if (op != Tensor_Operator::None)
            {
                X::Value leftValue = pTensor->GetLeftValue();

                // Process left value first if it's a tensor expression
                if (leftValue.IsObject() && leftValue.GetObj()->GetType() == ObjType::TensorExpression)
                {
                    TensorExpression* pLeft = dynamic_cast<TensorExpression*>(leftValue.GetObj());
                    if (pLeft && !pGraphBuildContext->IsCalledBuild(pLeft))
                    {
                        pGraphBuildContext->SetBuildCalled(pLeft);
                        BuildGraph(pBuildContext, pContext, pLeft, thisLevel_Action);
                    }
                }

                // Get flow information for this tensor expression
                X::AST::Expression* currentLineExpr = pTensor->GetCurrentLine();
                X::AST::If* pIfStmt = nullptr;
                int branchId = -1;
                bool isInIfStmt = false;

                // Use the branch information from tensor expression if available
                const FlowBranchInfo& branchInfo = pTensor->GetBranchInfo();
                if (branchInfo.ifEntry != nullptr) {
                    isInIfStmt = true;
                    pIfStmt = branchInfo.ifEntry;
                    branchId = branchInfo.branchId;
                }
                else {
                    isInIfStmt = IsInIfStatement(currentLineExpr, &pIfStmt, &branchId);
                }

                // Process the operation based on right value
                X::Value rightValue = pTensor->GetRightValue();
                if (rightValue.IsObject())
                {
                    XObj* pXObj = rightValue.GetObj();
                    if (pXObj->GetType() == ObjType::Tensor || pXObj->GetType() == ObjType::TensorExpression)
                    {
                        if (pXObj->GetType() == ObjType::TensorExpression)
                        {
                            TensorExpression* pRight = dynamic_cast<TensorExpression*>(pXObj);
                            GraphBuildAction action0 = GraphBuildAction::None;
                            if (!pGraphBuildContext->IsCalledBuild(pRight))
                            {
                                pGraphBuildContext->SetBuildCalled(pRight);
                                BuildGraph(pBuildContext, pContext, pRight, action0);
                            }
                        }
                        if (thisLevel_Action != GraphBuildAction::MeetBinaryOp)
                        {
                            // Check if the op is registered
                            auto opHandler = QueryRegisteredOpHandler(pBuildContext, pContext, (int)op);
                            if (opHandler)
                            {
                                X::ARGS inputs(2);
                                inputs.push_back(leftValue);
                                inputs.push_back(rightValue);
                                X::Value retValue(pTensor);
                                std::string strName = GetNameWithOp(op);

                                // Create a TensorRunItem with flow information
                                TensorRunItem item;
                                item.name = strName;
                                item.handler = opHandler;
                                item.inputs = inputs;
                                item.output = retValue;

                                // Set flow information if this is inside an If statement
                                if (isInIfStmt && pIfStmt) {
                                    // Check if this If has a parent If
                                    X::AST::If* parentIfStmt = nullptr;
                                    int parentBranchId = -1;
                                    bool hasParentIf = IsInIfStatement(pIfStmt->GetParent(), &parentIfStmt, &parentBranchId);

                                    unsigned long long flowId = 0;
                                    if (hasParentIf && parentIfStmt) {
                                        // First get/create the parent flow block
                                        unsigned long long parentFlowId = GetOrCreateFlowBlock(parentIfStmt);

                                        // Then get/create this flow block
                                        flowId = GetOrCreateFlowBlock(pIfStmt, parentFlowId, parentBranchId);
                                    }
                                    else {
                                        // Get/create a top-level flow block
                                        flowId = GetOrCreateFlowBlock(pIfStmt);
                                    }

                                    // Set flow information
                                    item.flowId = flowId;
                                    item.branchId = branchId;
                                }

                                // Add to the run list
                                m_runItems.push_back(item);
                            }
                        }
                    }
                    else if (pXObj->GetType() == ObjType::TensorOperator)
                    {
                        TensorOperator* pOp = dynamic_cast<TensorOperator*>(rightValue.GetObj());
                        if (pOp)
                        {
                            if (pOp->IsUnaryOp())
                            {
                                auto opHandler = pOp->GetOpHandler();
                                X::ARGS inputs(1);
                                inputs.push_back(leftValue);
                                X::Value retValue(pTensor);

                                // Create a TensorRunItem with flow information
                                TensorRunItem item;
                                item.name = pOp->GetName();
                                item.handler = opHandler;
                                item.inputs = inputs;
                                item.output = retValue;

                                // Set flow information if this is inside an If statement
                                if (isInIfStmt && pIfStmt) {
                                    // Check if this If has a parent If
                                    X::AST::If* parentIfStmt = nullptr;
                                    int parentBranchId = -1;
                                    bool hasParentIf = IsInIfStatement(pIfStmt->GetParent(), &parentIfStmt, &parentBranchId);

                                    unsigned long long flowId = 0;
                                    if (hasParentIf && parentIfStmt) {
                                        // First get/create the parent flow block
                                        unsigned long long parentFlowId = GetOrCreateFlowBlock(parentIfStmt);

                                        // Then get/create this flow block
                                        flowId = GetOrCreateFlowBlock(pIfStmt, parentFlowId, parentBranchId);
                                    }
                                    else {
                                        // Get/create a top-level flow block
                                        flowId = GetOrCreateFlowBlock(pIfStmt);
                                    }

                                    // Set flow information
                                    item.flowId = flowId;
                                    item.branchId = branchId;
                                }

                                // Add to the run list
                                m_runItems.push_back(item);
                            }
                            else
                            {
                                // Binary Op, need to use up level's right Value as right value
                                retAction = GraphBuildAction::MeetBinaryOp;
                                //TODO: ChatGPT ask to use line below?
                                //thisLevel_Action = GraphBuildAction::MeetBinaryOp;

                            }
                        }
                    }
                }
                else
                {
                    // Scalar such as int64/double
                    auto opHandler = QueryRegisteredOpHandler(pBuildContext, pContext, (int)op);
                    if (opHandler)
                    {
                        X::ARGS inputs(2);
                        inputs.push_back(leftValue);
                        inputs.push_back(rightValue);
                        X::Value retValue(pTensor);
                        std::string strName = GetNameWithOp(op);

                        // Create a TensorRunItem with flow information
                        TensorRunItem item;
                        item.name = strName;
                        item.handler = opHandler;
                        item.inputs = inputs;
                        item.output = retValue;

                        // Set flow information if this is inside an If statement
                        if (isInIfStmt && pIfStmt) {
                            // Check if this If has a parent If
                            X::AST::If* parentIfStmt = nullptr;
                            int parentBranchId = -1;
                            bool hasParentIf = IsInIfStatement(pIfStmt->GetParent(), &parentIfStmt, &parentBranchId);

                            unsigned long long flowId = 0;
                            if (hasParentIf && parentIfStmt) {
                                // First get/create the parent flow block
                                unsigned long long parentFlowId = GetOrCreateFlowBlock(parentIfStmt);

                                // Then get/create this flow block
                                flowId = GetOrCreateFlowBlock(pIfStmt, parentFlowId, parentBranchId);
                            }
                            else {
                                // Get/create a top-level flow block
                                flowId = GetOrCreateFlowBlock(pIfStmt);
                            }

                            // Set flow information
                            item.flowId = flowId;
                            item.branchId = branchId;
                        }

                        // Add to the run list
                        m_runItems.push_back(item);
                    }
                }
                if (thisLevel_Action == GraphBuildAction::MeetBinaryOp)
                {
                    // In this case, left must be TensorExpression
                    TensorExpression* pLeft = dynamic_cast<TensorExpression*>(leftValue.GetObj());
                    X::Value leftValue_LowLevel = pLeft->GetLeftValue();
                    X::Value opValue = pLeft->GetRightValue();
                    if (opValue.IsObject())
                    {
                        TensorOperator* pOp = dynamic_cast<TensorOperator*>(opValue.GetObj());
                        auto opHandler = pOp->GetOpHandler();
                        X::ARGS inputs(2);
                        inputs.push_back(leftValue_LowLevel);
                        inputs.push_back(rightValue);
                        X::Value retValue(pTensor);

                        // Create a TensorRunItem with flow information
                        TensorRunItem item;
                        item.name = pOp->GetName();
                        item.handler = opHandler;
                        item.inputs = inputs;
                        item.output = retValue;

                        // Set flow information if this is inside an If statement
                        if (isInIfStmt && pIfStmt) {
                            // Check if this If has a parent If
                            X::AST::If* parentIfStmt = nullptr;
                            int parentBranchId = -1;
                            bool hasParentIf = IsInIfStatement(pIfStmt->GetParent(), &parentIfStmt, &parentBranchId);

                            unsigned long long flowId = 0;
                            if (hasParentIf && parentIfStmt) {
                                // First get/create the parent flow block
                                unsigned long long parentFlowId = GetOrCreateFlowBlock(parentIfStmt);

                                // Then get/create this flow block
                                flowId = GetOrCreateFlowBlock(pIfStmt, parentFlowId, parentBranchId);
                            }
                            else {
                                // Get/create a top-level flow block
                                flowId = GetOrCreateFlowBlock(pIfStmt);
                            }

                            // Set flow information
                            item.flowId = flowId;
                            item.branchId = branchId;
                        }

                        // Add to the run list
                        m_runItems.push_back(item);
                    }
                }
            }

            // After processing the main operation (if any), process any sub-expressions (branches)
            auto& subs = pTensor->GetSubExpressions();
            for (auto& subExpr : subs) {
                if (subExpr.IsObject() && subExpr.GetObj()->GetType() == ObjType::TensorExpression) {
                    TensorExpression* pSubExpr = dynamic_cast<TensorExpression*>(subExpr.GetObj());
                    if (pSubExpr && !pGraphBuildContext->IsCalledBuild(pSubExpr)) {
                        pGraphBuildContext->SetBuildCalled(pSubExpr);
                        GraphBuildAction subAction = GraphBuildAction::None;
                        BuildGraph(pBuildContext, pContext, pSubExpr, subAction);
                    }
                }
            }
        }
	} //namespace Data
}//namespace X