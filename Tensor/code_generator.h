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

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <functional>
#include <unordered_set>
#include "tensor_expression.h"

namespace X {
    namespace Data {

        class TensorGraph;
        // Code generator using TensorRunItem's built-in handlers
        class CodeGenerator {
			TensorGraph* m_pGraph;
        public:
            // Constructor
			CodeGenerator() { m_pGraph = nullptr; }
            CodeGenerator(TensorGraph* g) { m_pGraph = g; }
            // Set handlers for structural code generation
            void setHeaderHandler(Tensor_OperatorHandler handler) { m_headerHandler = handler; }
            void setTrailerHandler(Tensor_OperatorHandler handler) { m_trailerHandler = handler; }
            void setBranchBeginHandler(Tensor_OperatorHandler handler) { m_branchBeginHandler = handler; }
            void setBranchEndHandler(Tensor_OperatorHandler handler) { m_branchEndHandler = handler; }

            // Generate the complete code
            std::string generate(X::Value& graph,
                const std::vector<TensorRunItem>& runItems,
                const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks);
        private:
            // Handlers for structural code generation
            Tensor_OperatorHandler m_headerHandler;
            Tensor_OperatorHandler m_trailerHandler;
            Tensor_OperatorHandler m_branchBeginHandler;
            Tensor_OperatorHandler m_branchEndHandler;

            // Helper method for handling flow transitions
            void handleFlowTransitions(
                X::Value& graph,
                std::stringstream& code,
                const TensorRunItem& item,
                std::stack<std::pair<unsigned long long, int>>& activeFlowStack,
                std::unordered_map<unsigned long long, std::vector<int>>& processedBranches,
                std::unordered_set<unsigned long long>& openedFlows,
                const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks);
            void ensureParentFlowsOpened(
                X::Value& graph,
                std::stringstream& code,
                const TensorRunItem& item,
                std::stack<std::pair<unsigned long long, int>>& activeFlowStack,
                std::unordered_map<unsigned long long, std::vector<int>>& processedBranches,
                std::unordered_set<unsigned long long>& openedFlows,
                const std::unordered_map<unsigned long long, FlowBlock>& flowBlocks);
		};
    } // namespace Data
} // namespace X