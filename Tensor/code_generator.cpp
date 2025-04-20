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
        std::string CodeGenerator::generate(
            X::ARGS& params, X::KWARGS& kwParams,
            X::Value& graph,
            const std::vector<TensorRunItem>& runItems,
            const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks)
        {
            std::stringstream code;

            // Generate header
            X::ARGS args(params.size() + 1);
            X::Value varFunc;
            X::Value headerValue;
            auto itFunc = kwParams.find("Func");
            if (itFunc)
            {
                varFunc = itFunc->val;
            }
            //first one is func,and then call parameters
            args.push_back(varFunc);
            for (auto& it : params)
            {
                args.push_back(it);
            }
            m_headerHandler(graph, args, headerValue);
            code << headerValue.ToString();

            // Track flow state
            std::stack<std::pair<unsigned long long, int>> activeFlowStack;
            std::unordered_map<unsigned long long, std::vector<int>> processedBranches;
            std::unordered_set<unsigned long long> openedFlows;

            for (size_t i = 0; i < runItems.size(); i++) {
                const auto& item = runItems[i];

                // Handle flow transitions
                handleFlowTransitions(graph, code, item, activeFlowStack,
                    processedBranches, openedFlows, flowBlocks);

                // Generate operation code with proper indentation
                int indentLevel = activeFlowStack.size() + 1;
                X::ARGS augmentedInputs = item.inputs;
                augmentedInputs.resize(augmentedInputs.size() + 1);
                augmentedInputs.push_back(X::Value(indentLevel));

                X::Value output = item.output;
                X::Value opResult = item.handler(graph, augmentedInputs, output);
                code << opResult.ToString();
            }

            // Close any remaining flow blocks
            while (!activeFlowStack.empty()) {
                X::ARGS endArgs(1);
                endArgs.push_back(X::Value(activeFlowStack.size()));
                X::Value endValue;
                m_branchEndHandler(graph, endArgs, endValue);
                code << endValue.ToString();
                activeFlowStack.pop();
            }

            // Generate trailer
            X::Value trailerValue;
            X::ARGS emptyArgs;
            m_trailerHandler(graph, emptyArgs, trailerValue);
            code << trailerValue.ToString();

            return code.str();
        }

        void CodeGenerator::handleFlowTransitions(
            X::Value& graph,
            std::stringstream& code,
            const TensorRunItem& item,
            std::stack<std::pair<unsigned long long, int>>& activeFlowStack,
            std::unordered_map<unsigned long long, std::vector<int>>& processedBranches,
            std::unordered_set<unsigned long long>& openedFlows,
            const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks)
        {
            // Skip flow handling for non-flow items
            if (item.flowId == 0) {
                // Close all active flows if we're returning to the main sequence
                while (!activeFlowStack.empty()) {
                    X::ARGS endArgs(1);
                    endArgs.push_back(X::Value(activeFlowStack.size()));
                    X::Value endValue;
                    m_branchEndHandler(graph, endArgs, endValue);
                    code << endValue.ToString();

                    activeFlowStack.pop();
                }
                openedFlows.clear();
                return;
            }

            // Build the complete flow hierarchy for the current item
            std::vector<std::pair<unsigned long long, int>> fullHierarchy;
            unsigned long long currentFlowId = item.flowId;
            int currentBranchId = item.branchId;

            // Add the current item's flow first
            fullHierarchy.push_back({ currentFlowId, currentBranchId });

            // Build the parent chain by traversing up
            while (flowBlocks.find(currentFlowId) != flowBlocks.end()) {
                const auto& block = flowBlocks.at(currentFlowId);
                if (block.parentId == 0) break;

                fullHierarchy.push_back({ block.parentId, block.parentBranchId });
                currentFlowId = block.parentId;
            }

            // Reverse to get root-to-leaf order
            std::reverse(fullHierarchy.begin(), fullHierarchy.end());

            // Align the active flow stack with the required hierarchy
            size_t matchingDepth = 0;

            // Find how much of the hierarchy already matches
            std::vector<std::pair<unsigned long long, int>> activeFlows;
            while (!activeFlowStack.empty()) {
                activeFlows.push_back(activeFlowStack.top());
                activeFlowStack.pop();
            }
            std::reverse(activeFlows.begin(), activeFlows.end());

            // Restore the stack
            for (const auto& flow : activeFlows) {
                activeFlowStack.push(flow);
            }

            // Find matching prefix without using std::min
            size_t compareLength = activeFlows.size();
            if (fullHierarchy.size() < compareLength) {
                compareLength = fullHierarchy.size();
            }

            for (size_t i = 0; i < compareLength; i++) {
                if (activeFlows[i].first == fullHierarchy[i].first &&
                    (activeFlows[i].second == fullHierarchy[i].second ||
                        i < fullHierarchy.size() - 1)) { // Allow branch mismatch except for leaf
                    matchingDepth = i + 1;
                }
                else {
                    break;
                }
            }

            // Close flows that don't match
            while (activeFlowStack.size() > matchingDepth) {
                auto [flowToClose, branchToClose] = activeFlowStack.top();

                X::ARGS endArgs(1);
                endArgs.push_back(X::Value(activeFlowStack.size()));
                X::Value endValue;
                m_branchEndHandler(graph, endArgs, endValue);
                code << endValue.ToString();

                if (activeFlowStack.size() == matchingDepth + 1) {
                    // Track processed branch if we're closing a branch but staying in the flow
                    if (matchingDepth > 0 && activeFlows[matchingDepth - 1].first == flowToClose) {
                        processedBranches[flowToClose].push_back(branchToClose);
                    }
                }

                openedFlows.erase(flowToClose);
                activeFlowStack.pop();
            }

            // Open flows that need to be added
            for (size_t i = matchingDepth; i < fullHierarchy.size(); i++) {
                auto [flowId, branchId] = fullHierarchy[i];

                // Skip if this exact flow+branch is already active
                if (!activeFlowStack.empty() &&
                    activeFlowStack.top().first == flowId &&
                    activeFlowStack.top().second == branchId) {
                    continue;
                }

                // Handle branch transition within same flow
                if (!activeFlowStack.empty() && activeFlowStack.top().first == flowId) {
                    auto [currentFlow, currentBranch] = activeFlowStack.top();

                    // Close current branch
                    X::ARGS endArgs(1);
                    endArgs.push_back(X::Value(activeFlowStack.size()));
                    X::Value endValue;
                    m_branchEndHandler(graph, endArgs, endValue);
                    code << endValue.ToString();

                    processedBranches[currentFlow].push_back(currentBranch);
                    activeFlowStack.pop();
                }

                // Determine branch type
                int branchType;
                if (branchId == -1) {
                    branchType = 2; // ELSE
                }
                else if (processedBranches[flowId].empty()) {
                    branchType = 0; // IF
                }
                else {
                    branchType = 1; // ELIF
                }

                // Generate branch begin code
                X::ARGS beginArgs(5);
                X::XPackageValue<AST::AstNode> valNode;
                AST::AstNode& astNode = *valNode;
                if (branchId >= 0) {
                    X::Value condition = m_pGraph->GetBlockCondition(flowId, branchId);
                    auto* refObj = dynamic_cast<X::Data::RefObject*>(condition.GetObj());
                    if (refObj) {
                        astNode.SetNode(refObj->GetExpression());
                    }
                }

                beginArgs.push_back(valNode);
                beginArgs.push_back(X::Value(branchType));
                beginArgs.push_back(X::Value(flowId));
                beginArgs.push_back(X::Value(branchId));
                beginArgs.push_back(X::Value(activeFlowStack.size() + 1)); // Proper indent level

                X::Value beginValue;
                m_branchBeginHandler(graph, beginArgs, beginValue);
                code << beginValue.ToString();

                openedFlows.insert(flowId);
                activeFlowStack.push({ flowId, branchId });
            }
        }
	}
}