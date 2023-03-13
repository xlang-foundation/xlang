#pragma once

#include "object.h"

namespace X
{
	namespace Data
	{
		struct TensorRunItem
		{
			Tensor_OperatorHandler handler;
			X::ARGS inputs;
			X::Value output;
		};
		class Tensor;
		class TensorGraph :
			virtual public XTensorGraph,
			virtual public Object
		{
			int m_LastInstructionId = 0;
			void BuildGraph(XObj* pContext, Tensor* pTensor);
		public:
			static void cleanup();
			TensorGraph():XTensorGraph(0),XObj(),Object()
			{
				m_t = ObjType::TensorGraph;
			}
			virtual void Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams) override;
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			bool Run(X::ARGS& params, X::KWARGS& kwParams);
		};
	}
}