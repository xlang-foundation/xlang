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
#include "exp.h"
#include "func.h"
#include <vector>
#include "decor.h"
#include "xlang.h"
#include "def.h"
#include "var.h"
#include "pair.h"

namespace X
{
	namespace AST
	{
		typedef X::Value(*Jit_Stub_Proc)(X::ARGS& vars);
		enum class JitType
		{
			Func,
			Class,
		};
		class JitBlock :
			public Func
		{
		protected:
			String m_JitCode;
			Jit_Stub_Proc m_Stub = nullptr;
			std::string m_strRetType;
			JitType  m_JitType = JitType::Func;
			String m_Name = { nullptr,0 };
			int m_Index = -1;//index for this JitBlock,set by compiling
			bool m_NameNeedRelease = false;
			List* Params = nullptr;
			std::vector<int> m_IndexofParamList;//same size with input positional param
			bool m_needSetHint = false;

			void SetName(Expression* n)
			{
				Var* vName = dynamic_cast<Var*>(n);
				if (vName)
				{
					m_Name = vName->GetName();
				}
				ReCalcHint(n);
			}
			virtual void SetParams(List* p)
			{
				if (m_needSetHint)
				{
					if (p)
					{
						auto& list = p->GetList();
						if (list.size() > 0)
						{
							ReCalcHint(list[0]);
							if (list.size() > 1)
							{
								ReCalcHint(list[list.size() - 1]);
							}
						}
					}
				}
				Params = p;
				if (Params)
				{
					//all parameters must set as left value
					auto& list = Params->GetList();
					for (auto& l : list)
					{
						l->SetIsLeftValue(true);
					}
					ReCalcHint(Params);
					Params->SetParent(this);
				}
			}
		public:
			JitBlock() :
				Func()
			{
				m_type = ObType::JitBlock;
			}
			JitBlock(JitType ty) :
				Func()
			{
				m_JitType = ty;
				m_type = ObType::JitBlock;
				m_pMyScope = new Scope();
				m_pMyScope->SetType((ty == JitType::Class)? ScopeType::Class:ScopeType::Func);
				m_pMyScope->SetExp(this);
			}
			~JitBlock()
			{
				if (Params) delete Params;
				if (m_NameNeedRelease)
				{
					delete m_Name.s;
				}
			}
			FORCE_INLINE void SetJitStub(Jit_Stub_Proc stub)
			{
				m_Stub = stub;
			}
			virtual void ScopeLayout() override;
			FORCE_INLINE std::string GetName()
			{
				return std::string(m_Name.s, m_Name.size);
			}
			FORCE_INLINE List* GetParams()
			{
				return Params;
			}
			FORCE_INLINE void SetRetType(std::string ty)
			{
				m_strRetType = ty;
			}
			FORCE_INLINE std::string& GetRetType()
			{
				return m_strRetType;
			}
			FORCE_INLINE void SetJitCode(String& code)
			{
				m_JitCode = code;
			}
			FORCE_INLINE std::string GetCode()
			{
				return std::string(m_JitCode.s,m_JitCode.size);
			}
			virtual void SetR(Expression* r) override
			{
				if (r->m_type == AST::ObType::Pair)
				{//have to be a pair
					AST::PairOp* pair = dynamic_cast<AST::PairOp*>(r);
					AST::Expression* paramList = pair->GetR();
					if (paramList)
					{
						if (paramList->m_type != AST::ObType::List)
						{
							AST::List* list = new AST::List(paramList);
							SetParams(list);
						}
						else
						{
							SetParams(dynamic_cast<AST::List*>(paramList));
						}
					}
					pair->SetR(nil);//clear R, because it used by SetParams
					AST::Expression* l = pair->GetL();
					if (l)
					{
						SetName(l);
						pair->SetL(nil);
					}
					//content used by func,and clear to nil,
					//not be used anymore, so delete it
					delete pair;
				}
				//only accept once
				NeedParam = false;
			}
			FORCE_INLINE virtual bool Call(XRuntime* rt, XObj* pContext,
				ARGS& params,
				KWARGS& kwParams,
				Value& retValue) override final
			{
				if (m_Stub == nullptr)
				{
					return false;
				}
				retValue = m_Stub(params);
				return true;
			}
			virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
				Value& v, LValue* lValue = nullptr) override;
		};
		class RetTypeOp :
			public UnaryOp
		{
		public:
			RetTypeOp() :
				UnaryOp()
			{
				m_type = ObType::ReturnType;
			}
			RetTypeOp(short op) :
				UnaryOp(op)
			{
				m_type = ObType::ReturnType;
			}
			virtual bool OpWithOperands(
				std::stack<AST::Expression*>& operands, int LeftTokenIndex)
			{
				//we need to check if there is JitBlock and an var in operands 
				if (operands.size() >= 2)
				{
					//we need to conside return type has multiple words such as 'unsinged long long'
					//so in operands,until find the JitBlock,we need to combine all operands after
					//JitBlock as return type
					Var* lastVar = nullptr;
					Var* firstVar = nullptr;
					while(!operands.empty())
					{
						auto* operand = operands.top();
						if (operand->m_type == AST::ObType::Var)
						{
							if (lastVar == nullptr)
							{
								lastVar = dynamic_cast<Var*>(operand);
							}
							firstVar = dynamic_cast<Var*>(operand);
						}
						else if (operand->m_type == AST::ObType::JitBlock)
						{
							break;
						}
						operands.pop();
					}
					auto operandL = operands.top();
					if (operandL && operandL->m_type == AST::ObType::JitBlock)
					{
						auto* jit = dynamic_cast<JitBlock*>(operandL);
						if (lastVar != nullptr)
						{
							if (firstVar == lastVar)//single word
							{
								jit->SetRetType(firstVar->GetNameString());
							}
							else//multiple words
							{
								auto& first_str = firstVar->GetName();
								auto& last_str = lastVar->GetName();
								std::string strRetType(first_str.s, last_str.s+last_str.size-first_str.s);
								jit->SetRetType(strRetType);
							}
						}
						//need to delete this object, we don't need to add into AST
						delete this;
						return true;
					}
				}
				else
				{
					//nobody need this node, so delete it
					delete this;
				}
				return true;
			}
		};
	}
}