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
#include "ref_object.h"

namespace X
{
    namespace Data
    {
        // Helper method to check if an expression is within an If statement
        bool TensorGraph::IsInIfStatement(
            X::AST::Expression* expr,
            X::AST::If** ppIfStmt,
            int* pBranchId)
        {
            if (!expr) return false;

            // Walk up the AST tree to find an If statement
            X::AST::Expression* current = expr;

            while (current) {
                if (current->m_type == X::AST::ObType::If) {
                    // Found an If statement - store the current If
                    X::AST::If* currentIf = static_cast<X::AST::If*>(current);

                    // Find the main 'if' by walking backward through the chain
                    X::AST::If* mainIf = currentIf;
                    while (mainIf && mainIf->GetPrev()) {
                        mainIf = mainIf->GetPrev();
                    }

                    // Update ppIfStmt to point to the main If
                    *ppIfStmt = mainIf;

                    // Determine branch ID based on position in the chain
                    if (currentIf == mainIf) {
                        // Current If is the main If - branch ID 0
                        *pBranchId = 0;
                    }
                    else {
                        // Check if it's an 'elif' or 'else'
                        if (currentIf->GetR()) {
                            // It has a condition, so it's an 'elif'
                            // Count steps from main If to current If
                            int position = 0;
                            X::AST::If* temp = mainIf;

                            while (temp && temp != currentIf) {
                                position++;
                                temp = temp->GetNext();
                            }

                            *pBranchId = position;
                        }
                        else {
                            // No condition means this is an 'else' branch
                            *pBranchId = -1;
                        }
                    }

                    return true;
                }

                // Move up the AST tree
                current = current->GetParent();
            }

            return false;
        }

        // Helper method to create or get a FlowBlock for an If statement
        unsigned long long TensorGraph::GetOrCreateFlowBlock(
            X::AST::If* pIfStmt,
            unsigned long long parentFlowId,
            int parentBranchId)
        {
            // Find the main 'if' statement (in case we were passed an elif/else)
            X::AST::If* mainIfStmt = pIfStmt;
            while (mainIfStmt && mainIfStmt->GetPrev()) {
                mainIfStmt = mainIfStmt->GetPrev();
            }

            // All branches of an if/elif/else share the same flow block ID
            // which is the ID of the main 'if' statement
            unsigned long long flowId = mainIfStmt->ID();

            auto it = m_flowBlocks.find(flowId);
            if (it == m_flowBlocks.end()) {
                // Create a new flow block
                FlowBlock block;
                block.id = flowId;
                block.parentId = parentFlowId;
                block.parentBranchId = parentBranchId;

                // Add the initial 'if' branch
                FlowBranch ifBranch;
                ifBranch.id = 0;

                // Get the condition expression for the 'if' branch
                X::AST::Expression* condExpr = mainIfStmt->GetR();
                if (condExpr) {
                    // Create a value for the condition
                    X::Data::RefObject* refObj = new X::Data::RefObject(condExpr);
                    ifBranch.condition = X::Value(refObj);
                }

                // Add the 'if' branch to the block
                block.branches.push_back(ifBranch);

                // Process 'elif'/'else' branches by following the next chain
                X::AST::If* nextStmt = mainIfStmt->GetNext();
                int branchId = 1;  // Start with 1 for first elif

                while (nextStmt) {
                    FlowBranch nextBranch;

                    // Get the condition expression
                    X::AST::Expression* nextCondExpr = nextStmt->GetR();

                    if (nextCondExpr) {
                        // This is an 'elif' branch
                        nextBranch.id = branchId++;

                        // Create a value for the condition
                        X::Data::RefObject* refObj = new X::Data::RefObject(nextCondExpr);
                        nextBranch.condition = X::Value(refObj);
                    }
                    else {
                        // No condition means this is an 'else' branch
                        nextBranch.id = -1;
                    }

                    // Add the branch to the block
                    block.branches.push_back(nextBranch);

                    // Move to the next branch in the chain
                    nextStmt = nextStmt->GetNext();
                }

                // Add the block to the map
                m_flowBlocks[flowId] = block;
            }

            return flowId;
        }
    }
}