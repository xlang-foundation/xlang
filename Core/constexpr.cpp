#include "constexpr.h"
#include "op.h"
namespace X
{
	namespace Data
	{
		void ConstExpr::Set(XlangRuntime* rt,AST::Expression* exp)
		{
			if (exp->m_type == AST::ObType::Param)
			{
				AST::Param* pParamExp = dynamic_cast<AST::Param*>(exp);
				//Condition
				Set(rt, pParamExp->GetName());
				//Action: Assgin
				Set(rt, pParamExp->GetType());
			}
			else if (exp->m_type == AST::ObType::Assign)
			{
				AST::Assign* pAssign = dynamic_cast<AST::Assign*>(exp);
				ExpOne one;
				one.op = pAssign->GetId();
				auto* pPair = dynamic_cast<AST::PairOp*>(pAssign->GetL());
				if (pPair)
				{
					if (pPair->GetR())
					{
						if (pPair->GetR()->m_type == AST::ObType::Number)
						{
							auto* pNum = dynamic_cast<AST::Number*>(pPair->GetR());
							if (pNum)
							{
								one.IndexOfArray = pNum->GetVal();
							}
						}
					}
				}
				auto* pR_Expr = pAssign->GetR();
				X::Value valRet;
				X::AST::ExecAction action;
				pR_Expr->Exec(rt,action, nullptr, valRet);
				one.val = valRet;
				m_exprs.push_back(one);
				m_hasAction = true;
			}
			else if (exp->m_type == AST::ObType::List)
			{
				AST::List* pListExp = dynamic_cast<AST::List*>(exp);
				auto& list = pListExp->GetList();
				for (auto* pItem : list)
				{
					Set(rt, pItem);
					m_lastCombineOp = OP_ID::Or;
				}
			}
			else if(exp->m_type == AST::ObType::BinaryOp)
			{
				auto* pBinOp = dynamic_cast<AST::BinaryOp*>(exp);
				if (pBinOp->GetL() 
					&& pBinOp->GetL()->m_type == AST::ObType::Pair
					&& pBinOp->GetR() 
					&& pBinOp->GetR()->m_type == AST::ObType::Number ||
					pBinOp->GetR()->m_type == AST::ObType::Str)
				{
					ExpOne one;
					one.withPrevious = m_lastCombineOp;
					one.op = pBinOp->GetId();
					auto* pPair = dynamic_cast<AST::PairOp*>(pBinOp->GetL());
					if (pPair)
					{
						if (pPair->GetR())
						{
							if (pPair->GetR()->m_type == AST::ObType::Number)
							{
								auto* pNum = dynamic_cast<AST::Number*>(pPair->GetR());
								if (pNum)
								{
									one.IndexOfArray = pNum->GetVal();
								}
							}
						}
					}
					auto* pExp_R = pBinOp->GetR();
					if (pExp_R->m_type == AST::ObType::Number)
					{
						auto* pNum = dynamic_cast<AST::Number*>(pExp_R);
						if (pNum)
						{
							one.val = pNum->GetVal();
						}
					}
					else if (pExp_R->m_type == AST::ObType::Double)
					{
						auto* pDouble = dynamic_cast<AST::Double*>(pExp_R);
						if (pDouble)
						{
							one.val = pDouble->GetVal();
						}
					}
					else if (pExp_R->m_type == AST::ObType::Str)
					{
						auto* pStr = dynamic_cast<AST::Str*>(pExp_R);
						if (pStr->IsCharSequence() && pStr->Size() == 1)
						{
							one.val = (long long)pStr->GetChars()[0];
						}
						else
						{
							//todo: check memory
							one.val = X::Value(pStr->GetChars(), pStr->Size());
						}
					}
					m_exprs.push_back(one);
				}
				else if(pBinOp->GetL() 
					&& pBinOp->GetL()->m_type == AST::ObType::BinaryOp
					&& pBinOp->GetR()
					&& pBinOp->GetR()->m_type == AST::ObType::BinaryOp)
				{//combine two logical Ops
					Set(rt, pBinOp->GetL());
					m_lastCombineOp = pBinOp->GetId();
					Set(rt, pBinOp->GetR());
				}
			}
		}
	}
}