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

			std::vector<TensorRunItem> m_runItems;

			Tensor_OperatorHandler QueryRegisteredOpHandler(void* pBuildContext,
				XObj* pPackage, int opIndex);
		public:
			static void cleanup();
			TensorGraph():XTensorGraph(0),XObj(),Object()
			{
				m_t = ObjType::TensorGraph;
			}
			virtual void Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams) override;
			virtual void GetBaseScopes(std::vector<AST::Scope*>& bases) override;
			bool Run(X::ARGS& params, X::KWARGS& kwParams);
			virtual std::string ToString(bool WithFormat = false) override;
		};
	}
}