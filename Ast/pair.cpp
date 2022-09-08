#include "pair.h"
#include "var.h"
#include "object.h"
#include "dict.h"
#include "list.h"
#include "table.h"

namespace X
{
namespace AST
{
bool PairOp::ParentRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
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
		Data::Object* obj = dynamic_cast<Data::Object*>(lVal.GetObj());
		if (obj)
		{
			bOK = obj->Call(rt, pContext,params, kwParams, v);
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
bool PairOp::GetItemFromDict(Runtime* rt, XObj* pContext,
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
bool PairOp::GetItemFromList(Runtime* rt, XObj* pContext,
	Data::List* pDataList, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	//Get Index
	std::vector<long long> IdxAry;
	if (R->m_type == ObType::List)
	{
		auto& list = (dynamic_cast<List*>(R))->GetList();
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
bool PairOp::BracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//usage: x[1,2]
		Value v0;
		bOK = L->Run(rt, pContext, v0);
		auto pDataObj = dynamic_cast<Data::Object*>(v0.GetObj());
		switch (pDataObj->GetType())
		{
		case X::ObjType::List:
			bOK = GetItemFromList(rt, pContext, 
				dynamic_cast<Data::List*>(pDataObj), R, v, lValue);
			break;
		case X::ObjType::Dict:
			bOK = GetItemFromDict(rt, pContext, dynamic_cast<Data::Dict*>(pDataObj), R, v, lValue);
			break;
		case X::ObjType::Table:
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
			auto& list = (dynamic_cast<List*>(R))->GetList();
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
		else if (R)
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
bool PairOp::CurlyBracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
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
			(dynamic_cast<Param*>(i))->GetName()->Run(rt, pContext, Key);
			(dynamic_cast<Param*>(i))->GetType()->Run(rt, pContext, Val);
			break;
		case ObType::Assign:
			(dynamic_cast<Assign*>(i))->GetL()->Run(rt, pContext, Key);
			(dynamic_cast<Assign*>(i))->GetL()->Run(rt, pContext, Val);
			break;
		case ObType::Var:
			(dynamic_cast<Var*>(i))->Run(rt, pContext, Key);
			break;
		default:
			break;
		}
		std::string strKey = Key.ToString();
		pDict->Set(Key, Val);
	};
	if (R->m_type == ObType::List)
	{
		auto& list = (dynamic_cast<AST::List*>(R))->GetList();
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
bool PairOp::TableBracketRun(Runtime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	Data::Table* pDataTable = new Data::Table();
	auto parsParam = [rt, pContext, pDataTable](Param* p) {
		auto name = p->GetName();
		std::string strName;
		std::vector<std::string> props;
		Value valDefaultValue;
		if (name && name->m_type == ObType::Var)
		{
			String& szName = (dynamic_cast<Var*>(name))->GetName();
			strName = std::string(szName.s, szName.size);
		}
		auto type0 = p->GetType();
		if (type0)
		{
			while (type0->m_type == ObType::Param)
			{
				auto p0 = dynamic_cast<Param*>(type0);
				auto name0 = p0->GetName();
				if (name0 && name0->m_type == ObType::Var)
				{
					String& sz0 = (dynamic_cast<Var*>(name0))->GetName();
					std::string str0 = std::string(sz0.s, sz0.size);
					props.push_back(str0);
				}
				type0 = p0->GetType();
			}
			if (type0->m_type == ObType::Assign)
			{
				auto assign0 = dynamic_cast<Assign*>(type0);
				auto name0 = assign0->GetL();
				if (name0 && name0->m_type == ObType::Var)
				{
					String& sz0 = (dynamic_cast<Var*>(name0))->GetName();
					std::string str0 = std::string(sz0.s, sz0.size);
					props.push_back(str0);
				}
				auto type1 = assign0->GetL();
				auto r = assign0->GetR();
				r->Run(rt, pContext, valDefaultValue);
			}
			else if (type0->m_type == ObType::Var)
			{
				String& sz0 = (dynamic_cast<Var*>(type0))->GetName();
				std::string str0 = std::string(sz0.s, sz0.size);
				props.push_back(str0);
			}
		}
		pDataTable->AddCol(strName, props, valDefaultValue);
	};
	if (R)
	{
		if (R->m_type == ObType::List)
		{
			auto& list = (dynamic_cast<List*>(R))->GetList();
			for (auto e : list)
			{
				if (e->m_type == ObType::Param)
				{
					parsParam(dynamic_cast<Param*>(e));
				}
				else
				{//todo: error

				}
			}
		}
		else if (R->m_type == ObType::Param)
		{
			parsParam(dynamic_cast<Param*>(R));
		}
		else
		{
			//todo:error
		}

	}
	v = Value(pDataTable);
	return true;
}
bool PairOp::Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue)
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
	case X::OP_ID::TableBracket_L:
		bOK = TableBracketRun(rt, pContext, v, lValue);
		break;
	default:
		break;
	}
	return bOK;
}
}
}