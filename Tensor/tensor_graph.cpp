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

namespace X
{
	namespace Data
	{
		static Obj_Func_Scope<1> _tensorGraphScope;
		void TensorGraph::Init()
		{
			_tensorGraphScope.Init();
			{
				auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
					X::ARGS& params,
					X::KWARGS& kwParams,
					X::Value& retValue)
				{
					TensorGraph* pGraph = dynamic_cast<TensorGraph*>(pContext);
					retValue = X::Value(pGraph->Run(params, kwParams));
					return true;
				};
				_tensorGraphScope.AddFunc("run", "graph.run()", f);
			}
			_tensorGraphScope.Close();
		}

		void TensorGraph::cleanup()
		{
			_tensorGraphScope.Clean();
		}
		void TensorGraph::GetBaseScopes(std::vector<AST::Scope*>& bases)
		{
			Object::GetBaseScopes(bases);
			bases.push_back(_tensorGraphScope.GetMyScope());
		}

		class GraphBuildContext
		{
			//match with Tensor_Operator in tensor.h
			std::vector<Tensor_OperatorHandler> m_Handlers;
			std::unordered_map<TensorExpression*, bool> m_BeCalledMap;
		public:
			GraphBuildContext()
			{
				Tensor_OperatorHandler dummy;
				for (int i = 0; i < (int)Tensor_Operator::Count; i++)
				{
					m_Handlers.push_back(dummy);
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
			bool IsCalledBuild(TensorExpression* exp)
			{
				auto it = m_BeCalledMap.find(exp);
				if (it != m_BeCalledMap.end())
				{
					return it->second;
				}
				else
				{
					return false;
				}
			}
			void SetBuildCalled(TensorExpression* exp)
			{
				m_BeCalledMap.emplace(std::make_pair(exp, true));
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
			auto* pObjPackage = dynamic_cast<X::Data::Object*>(pPackage);
			AST::Scope* pScope = pObjPackage->GetMyScope();
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
				SCOPE_FAST_CALL_AddOrGet0(idx,pScope,strName, true);
				if (idx >= 0)
				{
					X::Value valFunc;
					if (pPackage->GetIndexValue(idx, valFunc))
					{
						if (valFunc.IsObject())
						{
							XObj* pObjHandler = valFunc.GetObj();
							X::ARGS args(0);
							X::KWARGS kwargs;
							X::Value valTensorOp;
							pObjHandler->Call(nullptr, pPackage, args, kwargs, valTensorOp);
							if (valTensorOp.IsObject())
							{
								TensorOperator* pTensorOp = dynamic_cast<TensorOperator*>(valTensorOp.GetObj());
								if (pTensorOp)
								{
									handler = pTensorOp->GetOpHandler();
									pGraphBuildContext->SetHandler(opIndex, handler);
								}
							}
						}
					}
				}
			}
			return handler;
		}
		static std::string GetNameWithOp(Tensor_Operator op)
		{
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
			return strName;
		}
		void TensorGraph::BuildGraph(void* pBuildContext,
			XObj* pContext, TensorExpression* pTensor, GraphBuildAction& retAction)
		{
			GraphBuildContext* pGraphBuildContext = (GraphBuildContext*)pBuildContext;
			GraphBuildAction thiLevel_Action = GraphBuildAction::None;
			Tensor_Operator op = pTensor->GetOp();
			X::Value leftValue = pTensor->GetLeftValue();
			//check if left is a tensor expresion, if it is, then call BuildGraph
			//Check if this Tensor Expresion is already build graph or not
			if (leftValue.IsObject() && leftValue.GetObj()->GetType() == ObjType::TensorExpression)
			{
				TensorExpression* pLeft = dynamic_cast<TensorExpression*>(leftValue.GetObj());
				if (pLeft)
				{
					if (!pGraphBuildContext->IsCalledBuild(pLeft))
					{
						pGraphBuildContext->SetBuildCalled(pLeft);
						BuildGraph(pBuildContext, pContext, pLeft, thiLevel_Action);
					}
				}
			}
			X::Value rightValue = pTensor->GetRightValue();
			if (rightValue.IsObject())
			{
				XObj* pXObj = rightValue.GetObj();
				if (pXObj->GetType() == ObjType::Tensor || pXObj->GetType() == ObjType::TensorExpression)
				{
					if (pXObj->GetType() == ObjType::TensorExpression)
					{
						TensorExpression* pRight = dynamic_cast<TensorExpression*>(pXObj);
						GraphBuildAction action0 = GraphBuildAction::None;
						if (!pGraphBuildContext->IsCalledBuild(pRight))
						{
							pGraphBuildContext->SetBuildCalled(pRight);
							BuildGraph(pBuildContext, pContext, pRight, action0);//action0 should be None with return
						}
					}
					if (thiLevel_Action != GraphBuildAction::MeetBinaryOp)
					{
						//check if the op is registered
						auto opHandler = QueryRegisteredOpHandler(pBuildContext, pContext, (int)op);
						if (opHandler)
						{
							X::ARGS inputs(2);
							inputs.push_back(leftValue);
							inputs.push_back(rightValue);
							X::Value retValue(pTensor);
							std::string strName = GetNameWithOp(op);
							//put this handler into runlist
							m_runItems.push_back({ strName,opHandler,inputs, retValue });
						}
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
							X::ARGS inputs(1);
							inputs.push_back(leftValue);
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
			else
			{//scala such as int64/double
				auto opHandler = QueryRegisteredOpHandler(pBuildContext, pContext, (int)op);
				if (opHandler)
				{
					X::ARGS inputs(2);
					inputs.push_back(leftValue);
					inputs.push_back(rightValue);
					X::Value retValue(pTensor);
					std::string strName = GetNameWithOp(op);
					//put this handler into runlist
					m_runItems.push_back({ strName,opHandler,inputs, retValue });
				}
			}
			if (thiLevel_Action == GraphBuildAction::MeetBinaryOp)
			{
				//in this case, left must be TensorExpression
				TensorExpression* pLeft = dynamic_cast<TensorExpression*>(leftValue.GetObj());
				X::Value leftValue_LowLevel = pLeft->GetLeftValue();
				X::Value opValue = pLeft->GetRightValue();
				if (opValue.IsObject())
				{
					TensorOperator* pOp = dynamic_cast<TensorOperator*>(opValue.GetObj());
					auto opHandler = pOp->GetOpHandler();
					X::ARGS inputs(2);
					inputs.push_back(leftValue_LowLevel);
					inputs.push_back(rightValue);
					X::Value retValue(pTensor);
					//add into list
					m_runItems.push_back({ pOp->GetName(),opHandler,inputs, retValue });
					//opHandler(inputs, retValue);
				}
			}
		}
		void TensorGraph::Create(XObj* pContext, X::ARGS& params, X::KWARGS& kwParams)
		{
			GraphBuildContext buildContext;
			//check tensor's m_op, if the tensor package(pContext) has
			//this operator impl., replace with that
			for (auto& p : params)
			{
				if (!p.IsObject() || p.GetObj()->GetType() != ObjType::TensorExpression)
				{
					continue;
				}
				TensorExpression* tensorExp = dynamic_cast<TensorExpression*>(p.GetObj());
				GraphBuildAction action = GraphBuildAction::None;
				BuildGraph(&buildContext, pContext, tensorExp, action);
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
		const char* TensorGraph::ToString(bool WithFormat)
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
					if (in.IsObject())
					{
						if (in.GetObj()->GetType() == ObjType::Tensor)
						{
							Tensor* pTensor = dynamic_cast<Tensor*>(in.GetObj());
							if (pTensor)
							{
								inputName = pTensor->GetName();
							}
						}
						else if (in.GetObj()->GetType() == ObjType::TensorExpression)
						{
							TensorExpression* pTensorExp = dynamic_cast<TensorExpression*>(in.GetObj());
							if (pTensorExp)
							{
								inputName = pTensorExp->GetName();
							}
						}
					}
					else
					{
						inputName = "_"; //for scalar 
					}
					lineOut += " " + inputName;
				}
				if (item.output.IsObject())
				{
					std::string outputName;
					if (item.output.GetObj()->GetType() == ObjType::Tensor)
					{
						Tensor* pTensor = dynamic_cast<Tensor*>(item.output.GetObj());
						if (pTensor)
						{
							outputName = pTensor->GetName();
						}
					}
					else if (item.output.GetObj()->GetType() == ObjType::TensorExpression)
					{
						TensorExpression* pTensorExp = dynamic_cast<TensorExpression*>(item.output.GetObj());
						if (pTensorExp)
						{
							outputName = pTensorExp->GetName();
						}
					}
					lineOut += " -> " + outputName;
				}
				outStr += lineOut + "\r\n";
			}
			return GetABIString(outStr);
		}
	}
}