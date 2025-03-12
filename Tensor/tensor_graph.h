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

namespace X
{
	namespace Data
	{
		struct TensorRunItem
		{
			std::string name;
			Tensor_OperatorHandler handler;
			X::ARGS inputs;
			X::Value output;
		};
		class TensorExpression;
		class TensorGraph :
			virtual public XTensorGraph,
			virtual public Object
		{
			enum class GraphBuildAction
			{		
				None,
				MeetBinaryOp,
			};
			int m_LastInstructionId = 0;
			void BuildGraph(void* pBuildContext,
				XObj* pContext, TensorExpression* pTensor, GraphBuildAction& retAction);

			X::Value m_runner;//which impl. ops for tensor
			std::vector<TensorRunItem> m_runItems;

			Tensor_OperatorHandler QueryRegisteredOpHandler(void* pBuildContext,
				XObj* pPackage, int opIndex);
		public:
			static void Init();
			static void cleanup();
			TensorGraph():XTensorGraph(0),XObj(),Object()
			{
				m_t = ObjType::TensorGraph;
			}
			virtual void Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams) override;
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			bool Run(X::ARGS& params, X::KWARGS& kwParams);
			virtual const char* ToString(bool WithFormat = false) override;
		};
	}
}