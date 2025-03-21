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

#include "code_generator.h"
#include "ast.h"
#include "tensor_graph.h"
#include "ref_object.h"

namespace X
{
	namespace Data
	{
        std::string CodeGenerator::generate(X::Value& graph, 
            const std::vector<TensorRunItem>& runItems,
            const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks) 
        {
            std::stringstream code;

            // Generate header
            X::ARGS emptyArgs;
            X::Value headerValue;
            m_headerHandler(graph,emptyArgs, headerValue);
            code << headerValue.ToString();

            // Track the currently active flow blocks and branches
            std::stack<std::pair<unsigned long long, int>> activeFlowStack;
            std::unordered_map<unsigned long long, std::vector<int>> processedBranches;

            // Process all items in order
            for (size_t i = 0; i < runItems.size(); i++) {
                const auto& item = runItems[i];

                // Handle flow transitions
                handleFlowTransitions(graph,code, item, activeFlowStack, processedBranches);

                // Get the current indentation level
                int indentLevel = activeFlowStack.empty() ? 1 : 2;

                // Add indentation information to inputs
                X::ARGS augmentedInputs = item.inputs;
                augmentedInputs.resize(augmentedInputs.size() + 1);// Copy the inputs
                augmentedInputs.push_back(X::Value(indentLevel));  // Add indentation level

                // Call the item's own handler to generate operation code
				X::Value output = item.output;
                X::Value opResult = item.handler(graph,augmentedInputs, output);
                code << opResult.ToString();
            }

            // Close any remaining flow blocks
            while (!activeFlowStack.empty()) {
                X::ARGS endArgs(1);
                endArgs.push_back(X::Value(1)); // indent level
                X::Value endValue;
                m_branchEndHandler(graph,endArgs, endValue);
                code << endValue.ToString();
                activeFlowStack.pop();
            }

            // Generate trailer
            X::Value trailerValue;
            m_trailerHandler(graph,emptyArgs, trailerValue);
            code << trailerValue.ToString();

            return code.str();
        }
        void CodeGenerator::handleFlowTransitions(
			X::Value& graph,
            std::stringstream& code,
            const TensorRunItem& item,
            std::stack<std::pair<unsigned long long, int>>& activeFlowStack,
            std::unordered_map<unsigned long long, std::vector<int>>& processedBranches)
        {
            // Check if we need to close any flow blocks or start a new branch
            if (!activeFlowStack.empty()) {
                auto [currentFlowId, currentBranchId] = activeFlowStack.top();

                // If this item belongs to a different flow block or branch, close the current one
                if (item.flowId != currentFlowId || (item.flowId == currentFlowId && item.branchId != currentBranchId)) {
                    // Close the current flow block/branch
                    X::ARGS endArgs(1);
                    endArgs.push_back(X::Value(1)); // indent level
                    X::Value endValue;
                    m_branchEndHandler(graph,endArgs, endValue);
                    code << endValue.ToString();

                    activeFlowStack.pop();

                    // If this item is in the same flow but different branch, we need to track the processed branch
                    if (item.flowId == currentFlowId && item.branchId != currentBranchId) {
                        processedBranches[currentFlowId].push_back(currentBranchId);
                    }
                }
            }

            // Handle the current item based on its flow status
            if (item.flowId > 0) {
                // Item is part of a flow block

                // Check if we need to start a new branch
                if (activeFlowStack.empty() || activeFlowStack.top().first != item.flowId) {
                    // Determine branch type
                    int branchType;
                    if (item.branchId == -1) {
                        // 'else' branch
                        branchType = 2; // ELSE
                    }
                    else if (processedBranches[item.flowId].empty()) {
                        // 'if' branch (first branch in this flow)
                        branchType = 0; // IF
                    }
                    else {
                        // 'elif' branch
                        branchType = 1; // ELIF
                    }

                    // Generate branch begin code
                    X::ARGS beginArgs(5);
                    X::XPackageValue<AST::AstNode> valNode;
                    AST::AstNode& astNode = *valNode;
                    if (item.branchId >= 0)
                    {
                        X::Value condition = m_pGraph->GetBlockCondition(item.flowId, item.branchId);
                        auto* refObj = dynamic_cast<X::Data::RefObject*>(condition.GetObj());
                        if (refObj)
                        {
                            astNode.SetNode(refObj->GetExpression());
                        }
                    }

					beginArgs.push_back(valNode); // condition expression
                    beginArgs.push_back(X::Value(branchType));          // branch type
                    beginArgs.push_back(X::Value(item.flowId));         // flow ID
                    beginArgs.push_back(X::Value(item.branchId));       // branch ID
                    beginArgs.push_back(X::Value(1));                   // indent level

                    X::Value beginValue;
                    m_branchBeginHandler(graph,beginArgs, beginValue);
                    code << beginValue.ToString();

                    // Push the current flow/branch onto the stack
                    activeFlowStack.push({ item.flowId, item.branchId });
                }
            }
        }

	}
}