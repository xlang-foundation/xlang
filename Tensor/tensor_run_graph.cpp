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
//#include "code_generator.h"

namespace X
{
	namespace Data
	{
		/*
		bool TensorGraph::Run(X::ARGS& params, X::KWARGS& kwParams)
		{
			int steps = (int)m_runItems.size();
			for (int i = 0; i < steps; i++)
			{
				auto& item = m_runItems[i];
				item.handler(item.inputs, item.output);
			}
			return true;
		}
		*/
		bool TensorGraph::Run(X::ARGS& params, X::KWARGS& kwParams)
		{
			X::Value graph(this);
			std::string code = m_gen.generate(params, kwParams,graph,m_runItems, m_flowBlocks);
			m_codeGenerated = code;
			return true;
		}
	} //namespace Data
}//namespace X