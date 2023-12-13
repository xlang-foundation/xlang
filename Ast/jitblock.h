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
					auto operandR = operands.top();
					operands.pop();
					auto operandL = operands.top();
					if (operandL->m_type == AST::ObType::JitBlock)
					{
						auto* jit = dynamic_cast<JitBlock*>(operandL);
						operandR->SetParent(jit);
						if (operandR->m_type == AST::ObType::Var)
						{
							auto* var = dynamic_cast<Var*>(operandR);
							jit->SetRetType(var->GetNameString());
						}
						delete operandR;
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