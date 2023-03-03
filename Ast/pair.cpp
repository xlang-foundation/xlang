#include "pair.h"
#include "var.h"
#include "object.h"
#include "dict.h"
#include "list.h"
#include "set.h"
#include "table.h"
#include "pyproxyobject.h"

namespace X
{
namespace AST
{
bool PairOp::ParentRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//Call Func
		Value lVal;
		ExecAction action;
		bOK = L->Exec(rt,action, pContext, lVal, lValue);
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
			ExecAction action;
			bOK = R->Exec(rt,action, pContext, v, lValue);
		}
	}
	return bOK;
}
bool PairOp::GetItemFromDict(XlangRuntime* rt, XObj* pContext,
	Data::Dict* pDataDict, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	Value key;
	ExecAction action;
	bOK = r->Exec(rt,action, pContext, key);
	if (bOK)
	{
		bOK = pDataDict->Get(key, v, lValue);
	}
	return bOK;
}
bool PairOp::GetItemFromList(XlangRuntime* rt, XObj* pContext,
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
			ExecAction action;
			if (e->Exec(rt,action, pContext, v1))
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
		ExecAction action;
		bOK = R->Exec(rt,action, pContext, vIdx);
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
bool PairOp::BracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//usage: x[1,2]
		Value v0;
		ExecAction action;
		bOK = L->Exec(rt,action, pContext, v0);
		if (!v0.IsObject())
		{
			return false;
		}
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
		case X::ObjType::PyProxyObject:
		{
			auto* pPyObj = dynamic_cast<Data::PyProxyObject*>(pDataObj);
			if (pPyObj)
			{
				Value vIdx;
				ExecAction action;
				bOK = R->Exec(rt,action, pContext, vIdx);
				pPyObj->GetItem(vIdx.GetLongLong(),v);
			}
		}
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
				ExecAction action;
				if (e->Exec(rt,action, pContext, v))
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
			ExecAction action;
			if (R->Exec(rt,action, pContext, v))
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
bool PairOp::CurlyBracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = true;
	bool isDict = false; 
	Data::Dict* pDict = new Data::Dict();
	Data::mSet* pSet = new Data::mSet();
	auto KeyProc = [=](Expression* keyExpr)
	{
		X::Value retVal;
		if (keyExpr->m_type == ObType::Var)
		{
			auto name = dynamic_cast<Var*>(keyExpr)->GetNameString();
			retVal = X::Value(name);
		}
		else
		{
			ExecAction action;
			keyExpr->Exec(rt,action, pContext, retVal);
		}
		return retVal;
	};
	auto SetKWProcDict = [=](Expression* i, Data::Dict* pDict)
	{
		Value Key;
		Value Val;
		ExecAction action;
		switch (i->m_type)
		{
		case ObType::Param:
			Key = KeyProc((dynamic_cast<Param*>(i))->GetName());
			(dynamic_cast<Param*>(i))->GetType()->Exec(rt,action, pContext, Val);
			break;
		case ObType::Assign:
			(dynamic_cast<Assign*>(i))->GetL()->Exec(rt,action, pContext, Key);
			(dynamic_cast<Assign*>(i))->GetL()->Exec(rt,action, pContext, Val);
			break;

		default:
			break;
		}
		std::string strKey = Key.ToString();
		pDict->Set(Key, Val);
	};
	auto FindDict = [=](Expression* i)
	{
		Value Key;
		Value Val;
		if (i->m_type == ObType::Param || i->m_type == ObType::Assign) 
			return true;
		else
			return false;
	};

	if (R && R->m_type == ObType::List)
	{
		auto& list = (dynamic_cast<AST::List*>(R))->GetList();

		for (auto& i : list)
		{
			isDict = FindDict(i);
			if (isDict)
				break;
		}

		for (auto& i : list)
		{
			if (isDict){
				SetKWProcDict(i,pDict);
			}
			else {
				Value Val;
				ExecAction action;
				i->Exec(rt,action, pContext, Val);
				pSet->Set(Val);
			}
		}
	}
	else if(R)
	{
		isDict = FindDict(R);
		if (isDict){
			SetKWProcDict(R,pDict);
		}
		else {
			Value Val;
			ExecAction action;
			R->Exec(rt,action, pContext, Val);
			pSet->Set(Val);
		}
	}
	if (isDict){
		v = Value(pDict);
	}	
	else {
		v = Value(pSet);
	}

	return bOK;
}
bool PairOp::TableBracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
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
				ExecAction action;
				r->Exec(rt,action, pContext, valDefaultValue);
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
bool PairOp::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue)
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