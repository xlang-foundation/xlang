#include "decor.h"
#include "pair.h"
#include "object.h"
#include "constexpr.h"

namespace X
{
	namespace AST
	{
		bool Decorator::GetParamList(XlangRuntime* rt, Expression* e, ARGS& params, KWARGS& kwParams)
		{
			if (e->m_type != ObType::List)
			{
				params.resize(1);
				params.push_back(Value(new Data::Expr(e)));
			}
			else
			{
				auto& list = (dynamic_cast<List*>(e))->GetList();
				params.resize(list.size());
				for (auto i : list)
				{
					params.push_back(Value(new Data::Expr(i)));
				}
			}
			return true;
		}
		bool Decorator::RunExp(XlangRuntime* rt,Value& v, LValue* lValue)
		{
			if (!R || R->m_type != ObType::Pair)
			{
				return false;
			}
			auto* pR = dynamic_cast<BinaryOp*>(R);
			if (pR->GetL() && pR->GetL()->m_type == ObType::Var)
			{
				auto* pVarL = dynamic_cast<Var*>(pR->GetL());
				if (pVarL && pVarL->GetNameString() == "constexpr")
				{
					Data::ConstExpr* pExpr = new Data::ConstExpr();
					pExpr->Set(rt, pR->GetR());
					v = X::Value(pExpr);
				}
			}
			return true;
		}
		bool Decorator::Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
		{
			if (pContext == nullptr)
			{
				return RunExp(rt,v,lValue);//not called from decorated objct
			}
			bool bOK = true;
			if (R->m_type == ObType::Pair)
			{
				PairOp* pPairOp = dynamic_cast<PairOp*>(R);
				auto* pairL = pPairOp->GetL();
				if (pairL)
				{//Call Func
					Value lVal;
					ExecAction action;
					bOK = pairL->Exec(rt, action,pContext, lVal, lValue);
					if (!bOK || !lVal.IsObject())
					{
						return bOK;
					}
					ARGS params(0);
					KWARGS kwParams;
					auto* pairR = pPairOp->GetR();
					if (pairR)
					{
						bOK = GetParamList(rt, pairR, params, kwParams);
						if (!bOK)
						{
							return bOK;
						}
					}
					Data::Object* obj = dynamic_cast<Data::Object*>(lVal.GetObj());
					if (obj)
					{
						X::Value valTrailer(pContext);
						bOK = obj->CallEx(rt, pContext, params, kwParams, valTrailer,v);
					}
				}
			}
			return bOK;
		}
	}
}