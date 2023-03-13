#include "tensor_graph.h"
#include "function.h"
#include "tensor.h"
#include "tensorop.h"
#include <iostream>

namespace X
{
	namespace Data
	{
		class TensorGraphScope :
			virtual public AST::Scope
		{
			AST::StackFrame* m_stackFrame = nullptr;
		public:
			TensorGraphScope() :
				Scope()
			{
				Init();
			}
			void clean()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
					m_stackFrame = nullptr;
				}
			}
			~TensorGraphScope()
			{
				if (m_stackFrame)
				{
					delete m_stackFrame;
				}
			}
			void Init()
			{
				m_stackFrame = new AST::StackFrame(this);
				const int func_cnt = 1;
				m_stackFrame->SetVarCount(func_cnt);
				std::string strName;
				{
					strName = "run";
					AST::ExternFunc* extFunc = new AST::ExternFunc(strName,
						"graph.run()",
						(X::U_FUNC)([](X::XRuntime* rt, XObj* pContext,
							X::ARGS& params,
							X::KWARGS& kwParams,
							X::Value& retValue)
							{
								TensorGraph* pGraph = dynamic_cast<TensorGraph*>(pContext);
								retValue = X::Value(pGraph->Run(params, kwParams));
								return true;
							}));
					auto* pFuncObj = new Function(extFunc);
					pFuncObj->IncRef();
					int idx = AddOrGet(strName, false);
					Value funcVal(pFuncObj);
					m_stackFrame->Set(idx, funcVal);
				}
			}
			// Inherited via Scope
			virtual Scope* GetParentScope() override
			{
				return nullptr;
			}
			virtual bool Set(XlangRuntime* rt, XObj* pContext, int idx, Value& v) override
			{
				m_stackFrame->Set(idx, v);
				return true;
			}
			virtual bool Get(XlangRuntime* rt, XObj* pContext, int idx, Value& v,
				LValue* lValue = nullptr) override
			{
				m_stackFrame->Get(idx, v, lValue);
				return true;
			}
		};
		static TensorGraphScope _TensorGraphScope;
		void TensorGraph::cleanup()
		{
			_TensorGraphScope.clean();
		}
		void TensorGraph::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(&_TensorGraphScope);
		}
		void TensorGraph::BuildGraph(XObj* pContext, Tensor* pTensor)
		{
			if (!pTensor->NeedCalc())
			{
				return;
			}
			Tensor_Operator op = pTensor->GetOp();
			X::Value rightValue = pTensor->GetRightValue();
			if (rightValue.IsObject() && rightValue.GetObj()->GetType()== ObjType::Tensor)
			{
				TensorOperator* pOp = dynamic_cast<TensorOperator*>(rightValue.GetObj());
				if (pOp)
				{
					if (pOp->IsUnaryOp())
					{
						auto opHandler = pOp->GetOpHandler();
						X::ARGS inputs;
						inputs.push_back(X::Value(pTensor));
						X::Value retValue(pTensor);
						//put this handler into runlist
						//opHandler(inputs, retValue);
					}
					X::Value op_rightValue = pOp->GetRightValue();
					if (op_rightValue.IsObject() && op_rightValue.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pOp_rightTensor = dynamic_cast<Tensor*>(op_rightValue.GetObj());
						if (pOp_rightTensor->NeedCalc())
						{

						}
						else
						{// it is a tensor already has value
							auto opHandler = pOp->GetOpHandler();

						}
					}
					else
					{
						std::cout << "other type";
					}
				}
			}
		}
		void TensorGraph::Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams)
		{
			//check tensor's m_op, if the tensor package(pContext) has
			//this operator impl., replace with that
			for (auto& p : params)
			{
				if (!p.IsObject() || p.GetObj()->GetType() != ObjType::Tensor)
				{
					continue;
				}
				Tensor* tensor =dynamic_cast<Tensor*>(p.GetObj());
				BuildGraph(pContext, tensor);
			}
		}
		bool TensorGraph::Run(X::ARGS& params, X::KWARGS& kwParams)
		{
			return true;
		}
	}
}