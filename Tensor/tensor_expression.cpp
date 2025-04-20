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
#include "tensor.h"
#include "tensor_expression.h"


namespace X
{
	namespace Data
	{
		void TensorExpression::SetCurrentLine(X::AST::Expression* line)
		{
			Tensor::SetCurrentLine(line);
			if (line == nullptr)
			{
				return;
			}
			X::AST::If* pIfStmt = nullptr;
			int branchId = -1;
			bool isInIfStmt = TensorGraph::IsInIfStatement(line, &pIfStmt, &branchId);
			if (isInIfStmt)
			{
				m_branch = { line,pIfStmt,branchId};
			}
		}
	}
}