#include "pair.h"
#include "var.h"
#include "object.h"
#include "dict.h"
#include "list.h"

namespace X
{
namespace AST
{
bool PairOp::ParentRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//Call Func
		Value lVal;
		bOK = L->Run(rt, pContext, lVal, lValue);
		if (!bOK || !lVal.IsObject())
		{
			return bOK;
		}
		ARGS params;
		KWARGS kwParams;
		if (R)
		{
			bOK = GetParamList(rt, R, params, kwParams);
			if (!bOK)
			{
				return bOK;
			}
		}
		Data::Object* obj = (Data::Object*)lVal.GetObj();
		if (obj)
		{
			bOK = obj->Call(rt, params, kwParams, v);
		}
	}
	else
	{
		if (R && R->m_type != ObType::List)
		{
			bOK = R->Run(rt, pContext, v, lValue);
		}
	}
	return bOK;
}
bool PairOp::GetItemFromDict(Runtime* rt, void* pContext,
	Data::Dict* pDataDict, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	Value key;
	bOK = r->Run(rt, pContext, key);
	if (bOK)
	{
		bOK = pDataDict->Get(key, v, lValue);
	}
	return bOK;
}
bool PairOp::GetItemFromList(Runtime* rt, void* pContext,
	Data::List* pDataList, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	//Get Index
	std::vector<long long> IdxAry;
	if (R->m_type == ObType::List)
	{
		auto& list = ((List*)R)->GetList();
		for (auto e : list)
		{
			Value v1;
			if (e->Run(rt, pContext, v1))
			{
				IdxAry.push_back(v1.GetLongLong());
			}
			else
			{
				bOK = false;
				break;
			}
		}
	}
	else
	{
		Value vIdx;
		bOK = R->Run(rt, pContext, vIdx);
		IdxAry.push_back(vIdx.GetLongLong());
	}
	if (bOK)
	{
		if (IdxAry.size() > 0)
		{
			pDataList->Get(IdxAry[0], v, lValue);
		}
	}
	return bOK;
}
bool PairOp::BracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//usage: x[1,2]
		Value v0;
		bOK = L->Run(rt, pContext, v0);
		auto pDataObj = (Data::Object*)v0.GetObj();
		switch (pDataObj->GetType())
		{
		case Data::Type::List:
			bOK = GetItemFromList(rt, pContext, 
				(Data::List*)pDataObj,R,v, lValue);
			break;
		case Data::Type::Dict:
			bOK = GetItemFromDict(rt, pContext, (Data::Dict*)pDataObj, R, v, lValue);
			break;
		default:
			break;
		}
	}
	else
	{//Create list with []
		bOK = true;
		Data::List* pDataList = new Data::List();
		if (R && R->m_type == ObType::List)
		{
			auto& list = ((List*)R)->GetList();
			for (auto e : list)
			{
				Value v;
				if (e->Run(rt, pContext, v))
				{
					pDataList->Add(rt, v);
				}
				else
				{
					bOK = false;
					break;
				}
			}
		}
		else if (R && R->m_type == ObType::Var)
		{
			Value v;
			if (R->Run(rt, pContext, v))
			{
				pDataList->Add(rt, v);
			}
			else
			{
				bOK = false;
			}
		}
		v = Value(pDataList);
	}
	return bOK;
}
bool PairOp::CurlyBracketRun(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	bool bOK = true;
	Data::Dict* pDict = new Data::Dict();

	auto SetKWProc = [=](Expression* i, Data::Dict* pDict)
	{
		Value Key;
		Value Val;
		switch (i->m_type)
		{
		case ObType::Param:
			((Param*)i)->GetName()->Run(rt, pContext, Key);
			((Param*)i)->GetType()->Run(rt, pContext, Val);
			break;
		case ObType::Assign:
			((Assign*)i)->GetL()->Run(rt, pContext, Key);
			((Assign*)i)->GetL()->Run(rt, pContext, Val);
			break;
		case ObType::Var:
			((Var*)i)->Run(rt, pContext, Key);
			break;
		default:
			break;
		}
		std::string strKey = Key.ToString();
		pDict->Set(Key, Val);
	};
	if (R->m_type == ObType::List)
	{
		auto& list = ((AST::List*)R)->GetList();
		for (auto& i : list)
		{
			SetKWProc(i,pDict);
		}
	}
	else
	{
		SetKWProc(R, pDict);
	}
	v = Value(pDict);
	return bOK;
}
bool PairOp::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	bool bOK = false;
	switch (opId)
	{
	case X::OP_ID::Parenthesis_L:
		bOK = ParentRun(rt, pContext, v, lValue);
		break;
	case X::OP_ID::Brackets_L:
		bOK = BracketRun(rt, pContext, v, lValue);
		break;
	case X::OP_ID::Curlybracket_L:
		bOK = CurlyBracketRun(rt, pContext, v, lValue);
		break;
	default:
		break;
	}
	return bOK;
}
}
}