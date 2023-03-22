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

		class GraphBuildContext
		{
			//match with Tensor_Operator in tensor.h
			std::vector<Tensor_OperatorHandler> m_Handlers;
		public:
			GraphBuildContext()
			{
				for (int i = 0; i < (int)Tensor_Operator::Count; i++)
				{
					m_Handlers.push_back(nullptr);
				}
			}
			Tensor_OperatorHandler QueryHandler(int idx)
			{
				return m_Handlers[idx];
			}
			void SetHandler(int idx, Tensor_OperatorHandler handler)
			{
				m_Handlers[idx] = handler;
			}
		};

		Tensor_OperatorHandler TensorGraph::QueryRegisteredOpHandler(void* pBuildContext,
			XObj* pPackage, int opIndex)
		{
			GraphBuildContext* pGraphBuildContext = (GraphBuildContext*)pBuildContext;
			Tensor_OperatorHandler handler = pGraphBuildContext->QueryHandler(opIndex);
			if (handler)
			{
				return handler;
			}
			AST::Scope* pScope = dynamic_cast<AST::Scope*>(pPackage);
			if (pScope)
			{
				std::string strName;
				switch ((Tensor_Operator)opIndex)
				{
				case X::Data::Tensor_Operator::None:
					break;
				case X::Data::Tensor_Operator::Add:
					strName = "add";
					break;
				case X::Data::Tensor_Operator::Minus:
					strName = "minus";
					break;
				case X::Data::Tensor_Operator::Mul:
					strName = "mul";
					break;
				case X::Data::Tensor_Operator::Div:
					strName = "div";
					break;
				default:
					break;
				}
				int idx = pScope->AddOrGet(strName, true);
				if (idx>=0)
				{
					X::Value valFunc;
					if (pPackage->GetIndexValue(idx, valFunc))
					{
						if (valFunc.IsObject())
						{
							XObj* pObjHandler = valFunc.GetObj();
							X::ARGS args;
							X::KWARGS kwargs;
							X::Value valTensorOp;
							pObjHandler->Call(nullptr, pPackage, args, kwargs, valTensorOp);
							if (valTensorOp.IsObject())
							{
								TensorOperator* pTensorOp = dynamic_cast<TensorOperator*>(valTensorOp.GetObj());
								if (pTensorOp)
								{
									handler = pTensorOp->GetOpHandler();
									pGraphBuildContext->SetHandler(opIndex,handler);
								}
							}
						}
					}
				}
			}
			return handler;
		}
		void TensorGraph::BuildGraph(void* pBuildContext,
			XObj* pContext, Tensor* pTensor, GraphBuildAction& retAction)
		{
			if (!pTensor->NeedCalc())
			{
				return;
			}
			GraphBuildAction thiLevel_Action = GraphBuildAction::None;
			Tensor_Operator op = pTensor->GetOp();
			X::Value leftValue = pTensor->GetLeftValue();//left must be tensor
			if (leftValue.IsObject() && leftValue.GetObj()->GetType() == ObjType::Tensor)
			{
				Tensor* pLeft = dynamic_cast<Tensor*>(leftValue.GetObj());
				if (pLeft->NeedCalc())
				{
					BuildGraph(pBuildContext,pContext, pLeft, thiLevel_Action);
				}
			}
			else
			{
				//error
				return;
			}
			X::Value rightValue = pTensor->GetRightValue();
			if (rightValue.IsObject())
			{
				XObj* pXObj = rightValue.GetObj();
				if (pXObj->GetType() == ObjType::Tensor)
				{
					Tensor* pRight = dynamic_cast<Tensor*>(pXObj);
					if (pRight->NeedCalc())
					{
						GraphBuildAction action0 = GraphBuildAction::None;
						BuildGraph(pBuildContext,pContext, pRight, action0);//action0 should be None with return
					}
					//check if the op is registered
					auto opHandler = QueryRegisteredOpHandler(pBuildContext,pContext,(int)op);
					if (opHandler)
					{
						X::ARGS inputs;
						inputs.push_back(leftValue);
						inputs.push_back(rightValue);
						X::Value retValue(pTensor);

						std::string strName;
						switch (op)
						{
						case X::Data::Tensor_Operator::None:
							break;
						case X::Data::Tensor_Operator::Add:
							strName = "add";
							break;
						case X::Data::Tensor_Operator::Minus:
							strName = "minus";
							break;
						case X::Data::Tensor_Operator::Mul:
							strName = "mul";
							break;
						case X::Data::Tensor_Operator::Div:
							strName = "div";
							break;
						default:
							break;
						}
						//put this handler into runlist
						m_runItems.push_back({ strName,opHandler,inputs, retValue });

					}
				}
				else if (pXObj->GetType() == ObjType::TensorOperator)
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
							m_runItems.push_back({ pOp->GetName(),opHandler,inputs, retValue });
							//opHandler(inputs, retValue);
						}
						else
						{//Binary Op,nned to use up level's right Value as right value
							retAction = GraphBuildAction::MeetBinaryOp;
						}
					}
				}
			}
			if (thiLevel_Action == GraphBuildAction::MeetBinaryOp)
			{
				Tensor* pLeft = dynamic_cast<Tensor*>(leftValue.GetObj());
				X::Value leftValue_LowLevel = pLeft->GetLeftValue();
				X::Value opValue = pLeft->GetRightValue();
				if (opValue.IsObject() && rightValue.GetObj()->GetType() == ObjType::Tensor)
				{
					TensorOperator* pOp = dynamic_cast<TensorOperator*>(opValue.GetObj());
					auto opHandler = pOp->GetOpHandler();
					X::ARGS inputs;
					inputs.push_back(leftValue_LowLevel);
					inputs.push_back(rightValue);
					X::Value retValue(pTensor);
					//add into list
					m_runItems.push_back({ pOp->GetName(),opHandler,inputs, retValue });
					//opHandler(inputs, retValue);
				}
			}
		}
		void TensorGraph::Create(XObj* pContext,X::ARGS& params, X::KWARGS& kwParams)
		{
			GraphBuildContext buildContext;
			//check tensor's m_op, if the tensor package(pContext) has
			//this operator impl., replace with that
			for (auto& p : params)
			{
				if (!p.IsObject() || p.GetObj()->GetType() != ObjType::Tensor)
				{
					continue;
				}
				Tensor* tensor =dynamic_cast<Tensor*>(p.GetObj());
				GraphBuildAction action = GraphBuildAction::None;
				BuildGraph(&buildContext,pContext, tensor, action);
			}
		}
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
		std::string TensorGraph::ToString(bool WithFormat)
		{
			std::string outStr;
			int steps = (int)m_runItems.size();
			for (int i = 0; i < steps; i++)
			{
				auto& item = m_runItems[i];
				std::string lineOut = item.name;
				for (auto& in : item.inputs)
				{
					std::string inputName;
					if (in.IsObject() && in.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pTensor = dynamic_cast<Tensor*>(in.GetObj());
						if(pTensor)
						{
							inputName = pTensor->GetName();
						}
					}
					else
					{
						inputName = "_"; //for scalar 
					}
					lineOut += " " + inputName;
				}
				if (item.output.IsObject() && item.output.GetObj()->GetType() == ObjType::Tensor)
				{
					std::string outputName;
					Tensor* pTensor = dynamic_cast<Tensor*>(item.output.GetObj());
					if (pTensor)
					{
						outputName = pTensor->GetName();
					}
					lineOut += " -> " + outputName;
				}
				outStr += lineOut + "\r\n";
			}
			return outStr;
		}
	}
}