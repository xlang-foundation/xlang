#include "dotop.h"
#include "object.h"
#include "var.h"
#include "number.h"
#include "list.h"
#include "function.h"
#include "prop.h"
#include "funclist.h"
#include "moduleobject.h"
#include "metascope.h"

namespace X
{
namespace AST
{

void DotOp::ScopeLayout()
{
	if (L) L->ScopeLayout();
	//R will be decided in run stage
}
void DotOp::QueryBases(Runtime* rt, Data::Object* pObj,
	std::vector<Scope*>& bases)
{
	pObj->GetBaseScopes(bases);
	bases.push_back(&MetaScope::I());
}
void DotOp::RunScopeLayoutWithScopes(Expression* pExpr,
	std::vector<Scope*>& scopes)
{
	if (pExpr->m_type == ObType::Pair)
	{
		PairOp* pPair = dynamic_cast<PairOp*>(pExpr);
		Expression* pair_r = pPair->GetR();
		if (pair_r && pair_r->m_type == ObType::List)
		{
			AST::List* pList = dynamic_cast<AST::List*>(pair_r);
			auto& list = pList->GetList();
			for (auto it : list)
			{
				if (it->m_type == ObType::Var)
				{
					Var* var = dynamic_cast<Var*>(it);
					var->ScopeLayout(scopes);
				}
			}
		}
		else if(pair_r && pair_r->m_type == ObType::Var)
		{
			Var* var = dynamic_cast<Var*>(pair_r);
			var->ScopeLayout(scopes);
		}
	}
	else if (pExpr->m_type == ObType::Var)
	{
		Var* var = dynamic_cast<Var*>(pExpr);
		var->ScopeLayout(scopes);
	}
}
bool DotOp::DotProcess(Runtime* rt, XObj* pContext, 
	Value& v_l, Expression* R,
	Value& v, LValue* lValue)
{
	std::vector<Scope*> scopes;
	auto* pLeftObj0 = dynamic_cast<Data::Object*>(v_l.GetObj());
	if (pLeftObj0)
	{
		QueryBases(rt, pLeftObj0, scopes);
	}
	//R can be a Var or List
	if (R)
	{
		RunScopeLayoutWithScopes(R, scopes);
	}

	Data::FuncCalls* pCallList = nil;
	Data::List* pValueList = nil;

	auto AddFunc = [&](
		Value& v0, LValue& lVal,
		XObj* pContext)
	{
		if (v0.IsObject())
		{
			Data::Object* pObj0 = dynamic_cast<Data::Object*>(v0.GetObj());
			if (pObj0 && pObj0->GetType() == X::ObjType::Function)
			{
				Data::Function* pFuncObj = dynamic_cast<Data::Function*>(pObj0);
				if (pFuncObj)
				{
					Func* func = pFuncObj->GetFunc();
					if (func)
					{
						if (pCallList == nil) pCallList = new Data::FuncCalls();
						pCallList->Add(pContext, func, nil);
					}
				}
			}
			else
			{
				lVal.SetContext(pContext);
				if (pValueList == nil)
				{
					pValueList = new Data::List();
				}
				pValueList->Add(lVal);
			}
		}
		else
		{
			if (pValueList == nil)
			{
				pValueList = new Data::List();
			}
			lVal.SetContext(pContext);
			pValueList->Add(lVal);
		}
	};
	auto RunCallPerObj = [&](
		Expression* pExpr,
		XObj* pContext)
	{
		if (pExpr->m_type == ObType::Pair)
		{
			PairOp* pPair = dynamic_cast<PairOp*>(pExpr);
			Expression* pair_r = pPair->GetR();
			if (pair_r && pair_r->m_type == ObType::List)
			{
				AST::List* pList = dynamic_cast<AST::List*>(pair_r);
				auto& list = pList->GetList();
				for (auto it : list)
				{
					if (it->m_type == ObType::Var)
					{
						Var* var = dynamic_cast<Var*>(it);
						Value v0;
						LValue lVal = nil;
						var->Run(rt, pContext, v0, &lVal);
						AddFunc(v0, lVal,pContext);
					}
				}
			}
			else if (pair_r && pair_r->m_type == ObType::Var)
			{
				Var* var = dynamic_cast<Var*>(pair_r);
				Value v0;
				LValue lVal = nil;
				var->Run(rt, pContext, v0, &lVal);
				AddFunc(v0, lVal,pContext);
			}
		}
		else if (pExpr->m_type == ObType::Var)
		{
			Var* var = dynamic_cast<Var*>(pExpr);
			Value v0;
			LValue lValue = nil;
			var->Run(rt, pContext, v0, &lValue);
			AddFunc(v0, lValue,pContext);
		}
	};
	if (pLeftObj0)
	{
		Data::Object* pLeftObj = (Data::Object*)pLeftObj0;
		Data::List* pList = dynamic_cast<Data::List*>(pLeftObj);
		if (pList)
		{
			auto& data = pList->Data();
			for (auto& it : data)
			{
				if (it.IsObject())
				{
					Data::Object* pItObj = dynamic_cast<Data::Object*>(it.GetObj());
					if (pItObj->GetType() == X::ObjType::XClassObject)
					{
						RunCallPerObj(R, pItObj);
					}
					else if (pItObj->GetType() == X::ObjType::Function)
					{
						RunCallPerObj(R, pItObj);
					}
				}
			}
		}
		else
		{
			RunCallPerObj(R,pLeftObj);
		}
	}
	//Function call first
	if (pCallList)
	{
		v = Value(pCallList);
		if (pValueList)
		{
			delete pValueList;
		}
	}
	else if (pValueList)
	{
		if (pValueList->Size() == 1)
		{//only one
			pValueList->Get(0, v, lValue);
			delete pValueList;
		}
		else
		{
			v = Value(pValueList);
		}
	}
	return true;
}
bool DotOp::Run(Runtime* rt,XObj* pContext,Value& v, LValue* lValue)
{
	if (!L || !R)
	{
		return false;
	}
	if (L->m_type == ObType::Number)
	{	
		Number* pLeftNum = dynamic_cast<Number*>(L);
		double dValue = (double)pLeftNum->GetVal();
		double fraction = 0;
		int digiNum = 0;
		if (R->m_type == ObType::Number)
		{
			Number* pNum = dynamic_cast<Number*>(R);
			fraction = (double)pNum->GetVal();
			digiNum = pNum->GetDigiNum();
		}
		else if(R->m_type == ObType::Var)
		{
			String name = (dynamic_cast<Var*>(R))->GetName();
			double dVal =0;
			long long llVal =0;
			ParseState st = ParseNumber(name, dVal, llVal);
			if (st == ParseState::Long_Long)
			{
				digiNum = (int)dVal;
				fraction = (double)llVal;
			}
		}
		else
		{//TODO: error
			return false;
		}
		//TODO:optimize here
		for (int i = 0; i < digiNum; i++)
		{
			fraction /= 10;
		}
		dValue += fraction;
		v = Value(dValue);
		return true;
	}
	Value v_l;
	if (!L->Run(rt, pContext, v_l) || !v_l.IsObject())
	{
		return false;
	}
	Expression* r = R;
	while (r->m_type == ObType::Dot)
	{
		DotOp* dotR = dynamic_cast<DotOp*>(r);
		Value v0;
		LValue lValue0=nil;
		DotProcess(rt, pContext, v_l, dotR->GetL(),v0,&lValue0);
		v_l = v0;
		r = dotR->GetR();
	}
	DotProcess(rt, pContext,v_l, r,v, lValue);
	return true;
}
bool DotOp::CalcCallables(Runtime* rt, XObj* pContext,
	std::vector<Scope*>& callables)
{
	Value val;
	bool bOK = Run(rt, pContext, val);
	if (bOK && val.IsObject())
	{
		bOK = dynamic_cast<Data::Object*>(val.GetObj())->CalcCallables(rt, pContext, callables);
	}
	return bOK;
}
}
}