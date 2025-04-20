﻿/*
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

#include "pair.h"
#include "var.h"
#include "object.h"
#include "dict.h"
#include "list.h"
#include "tensor.h"
#include "set.h"
#include "table.h"
#include "pyproxyobject.h"
#include "bin.h"

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
		bOK = ExpExec(L,rt,action, pContext, lVal, lValue);
		if (!bOK)
		{
			return bOK;
		}
		//to support this case :)
		//x =(3+4)(), for this one, xlang thinks it is just like x =3+4
		if (!lVal.IsObject())
		{
			v = lVal;
		}
		else
		{
			ARGS params(0);
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
				bOK = obj->Call(rt, pContext, params, kwParams, v);
			}
		}
	}
	else if(R)
	{
		if (R->m_type == ObType::List)
		{
			bOK = true;
			X::List valList;
			auto& list = (dynamic_cast<List*>(R))->GetList();
			for (auto e : list)
			{
				Value v1;
				ExecAction action;
				if (ExpExec(e, rt, action, pContext, v1))
				{
					valList += v1;
				}
				else
				{
					bOK = false;
					break;
				}
			}
			v = Value(valList);
		}
		else
		{
			ExecAction action;
			bOK = ExpExec(R,rt,action, pContext, v, lValue);
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
	bOK = ExpExec(r,rt,action, pContext, key);
	if (bOK)
	{
		//for assign like d["key1"] = value
		if (IsLeftValue() || r->IsLeftValue())
		{
			bOK = pDataDict->GetLValueToAssign(key,v);
		}
		else
		{
			bOK = pDataDict->Get(key, v, lValue);
		}
	}
	return bOK;
}
bool PairOp::GetItemFromTensor(XlangRuntime* rt, XObj* pContext,
	Data::Tensor* pTensor, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
#if not defined(BARE_METAL)
	auto extract_from_param = [&](Param* pParam)
	{
		Data::TensorIndex retIdx = { 0,-1 };
		Value vIdx;
		ExecAction action;
		auto* first = pParam->GetName();
		auto* second = pParam->GetType();
		if (first && ExpExec(first,rt, action, pContext, vIdx))
		{
			retIdx.i = (long long)vIdx;
		}
		if (second && ExpExec(second,rt, action, pContext, vIdx))
		{
			retIdx.j = (long long)vIdx;
		}
		return retIdx;
	};

	std::vector<Data::TensorIndex> IdxAry;
	if (R->m_type == ObType::List)
	{
		auto& list = (dynamic_cast<List*>(R))->GetList();
		for (auto e : list)
		{
			Data::TensorIndex idx = { 0,-1 };
			if (e->m_type == ObType::Param)
			{
				Param* pParam = dynamic_cast<Param*>(e);
				idx = extract_from_param(pParam);
			}
			else
			{
				//keep index start with end as same number
				Value v1;
				ExecAction action;
				if (ExpExec(e,rt, action, pContext, v1))
				{
					idx.i = idx.j = (long long)v1;
				}
			}
			IdxAry.push_back(idx);
		}
	}
	else if (R->m_type == ObType::Param)
	{
		Param* pParam = dynamic_cast<Param*>(R);
		Data::TensorIndex idx = extract_from_param(pParam);
		IdxAry.push_back(idx);
	}
	else
	{
		Data::TensorIndex idx = { 0,-1 };
		Value v1;
		ExecAction action;
		if (ExpExec(R,rt, action, pContext, v1))
		{
			idx.i = idx.j = (long long)v1;
		}
		IdxAry.push_back(idx);
	}
	bOK = pTensor->Get(IdxAry,v);
#endif
	return bOK;
}
bool PairOp::GetItemFromList(XlangRuntime* rt, XObj* pContext,
	Data::List* pDataList, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	bool isRange = false;
	//Get Index
	std::vector<long long> IdxAry;
	switch (R->m_type)
	{
	case ObType::List:
	{
		auto& list = (dynamic_cast<List*>(R))->GetList();
		for (auto e : list)
		{
			Value v1;
			ExecAction action;
			if (ExpExec(e, rt, action, pContext, v1))
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
	break;
	case ObType::Param:
	{
		auto* param = dynamic_cast<Param*>(R);
		if (param->GetName())
		{
			Value vIdx;
			ExecAction action;
			bOK = ExpExec(param->GetName(), rt, action, pContext, vIdx);
			IdxAry.push_back(vIdx.GetLongLong());
		}
		else
		{
			IdxAry.push_back(0);
		}
		if (param->GetType())
		{
			Value vIdx;
			ExecAction action;
			bOK = ExpExec(param->GetType(), rt, action, pContext, vIdx);
			IdxAry.push_back(vIdx.GetLongLong());
		}
		else
		{
			IdxAry.push_back(-1);
		}
		isRange = true;
		bOK = true;
	}
	break;
	default:
	{
		Value vIdx;
		ExecAction action;
		bOK = ExpExec(R, rt, action, pContext, vIdx);
		IdxAry.push_back(vIdx.GetLongLong());
	}
	break;
	}

	if (bOK)
	{
		if (IdxAry.size() ==1)
		{
			pDataList->Get(IdxAry[0], v, lValue);
		}
		else if (isRange && IdxAry.size() == 2)
		{
			pDataList->GetRange(IdxAry[0], IdxAry[1], v, lValue);
		}
		else if(IdxAry.size()>0)
		{
			//get each of them and return a list
			Data::List* pList = new Data::List();
			for (auto idx : IdxAry)
			{
				Value v1;
				pDataList->Get(idx, v1);
				pList->Add(rt, v1);
			}
			v = Value(pList);
		}
		else
		{
			bOK = false;

		}
	}
	return bOK;
}
bool PairOp::GetItemFromBin(XlangRuntime* rt, 
	XObj* pContext, Data::Binary* pDataBin, 
	Expression* r, Value& v, LValue* lValue)
{
	bool bOK = true;
	
	long long idx1 = 0;
	long long idx2 = -1;
	bool isRange = false;
	auto size = pDataBin->Size();

	switch (R->m_type)
	{
	case ObType::Param:
	{
		auto* param = dynamic_cast<Param*>(R);
		if (param->GetName())
		{
			Value vIdx;
			ExecAction action;
			bOK = ExpExec(param->GetName(), rt, action, pContext, vIdx);
			idx1 = vIdx.GetLongLong();
		}
		else
		{
			idx1 = 0;
		}
		if (param->GetType())
		{
			Value vIdx;
			ExecAction action;
			bOK = ExpExec(param->GetType(), rt, action, pContext, vIdx);
			idx2 = vIdx.GetLongLong();
		}
		else
		{
			idx2 = size-1;
		}
		isRange = true;
		bOK = true;
	}
	break;
	default:
	{
		Value vIdx;
		ExecAction action;
		bOK = ExpExec(R, rt, action, pContext, vIdx);
		idx1 = vIdx.GetLongLong();
	}
	break;
	}

	if (bOK)
	{
		char* data = pDataBin->Data();
		if (idx1 >=0 && idx1 <size)
		{
			if (isRange && idx2 >= 0 && idx2 < size)
			{
				auto len = idx2 - idx1 + 1;
				if (len > 0)
				{
					char* pNewData = new char[len];
					memcpy(pNewData, data + idx1, len);
					v = Value(new Data::Binary(pNewData, len,true));
				}
				else
				{
					v = Value();
				}
			}
			else
			{
				v = Value(data[idx1]);
			}
		}
		else
		{
			bOK = false;

		}
	}
	return bOK;
}
bool PairOp::GetItemFromPackage(XlangRuntime* rt, XObj* pContext,
	Data::Object* pPackage, Expression* r,
	Value& v, LValue* lValue)
{
	bool bOK = true;
	//Get Index
	X::Port::vector<X::Value> IdxAry(0);
	if (R->m_type == ObType::List)
	{
		auto& list = (dynamic_cast<List*>(R))->GetList();
		IdxAry.resize(list.size());
		for (auto e : list)
		{
			Value v1;
			ExecAction action;
			if (ExpExec(e,rt,action, pContext, v1))
			{
				IdxAry.push_back(v1);
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
		IdxAry.resize(1);
		Value vIdx;
		ExecAction action;
		bOK = ExpExec(R,rt,action, pContext, vIdx);
		IdxAry.push_back(vIdx);
	}
	if (bOK)
	{
		bOK = dynamic_cast<X::XObj*>(pPackage)->Get(rt, pPackage, IdxAry, v);
	}
	return bOK;
}
//we can split this impl. into Object's derived classes, but 
//that will increase its vtable and Object's memory size will become bigger
// also if do that way, we need to run expression for R in these classes
//so still keep here
bool PairOp::BracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = false;
	if (L)
	{//usage: x[1,2] or x[a:b]
		Value v0;
		ExecAction action;
		bOK = ExpExec(L,rt,action, pContext, v0);
		if (!v0.IsObject())
		{
			return false;
		}
		auto pDataObj = dynamic_cast<Data::Object*>(v0.GetObj());
		switch (pDataObj->GetType())
		{
		case X::ObjType::Package:
			bOK = GetItemFromPackage(rt, pContext,
				pDataObj, R, v, lValue);
			break;
		case X::ObjType::List:
			bOK = GetItemFromList(rt, pContext, 
				dynamic_cast<Data::List*>(pDataObj), R, v, lValue);
			break;
		case X::ObjType::Binary:
			bOK = GetItemFromBin(rt, pContext,
				dynamic_cast<Data::Binary*>(pDataObj), R, v, lValue);
			break;
		case X::ObjType::Dict:
			bOK = GetItemFromDict(rt, pContext, dynamic_cast<Data::Dict*>(pDataObj), R, v, lValue);
			break;
		case X::ObjType::Table:
			break;
#if not defined(BARE_METAL)
		case X::ObjType::Tensor:
		case X::ObjType::TensorExpression:
			bOK = GetItemFromTensor(rt, pContext,
				dynamic_cast<Data::Tensor*>(pDataObj), R, v, lValue);
			break;
#endif
		case X::ObjType::PyProxyObject:
		{
			auto* pPyObj = dynamic_cast<Data::PyProxyObject*>(pDataObj);
			if (pPyObj)
			{
				Value vIdx;
				ExecAction action;
				bOK = ExpExec(R,rt,action, pContext, vIdx);
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
				if (ExpExec(e,rt,action, pContext, v))
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
			if (ExpExec(R,rt,action, pContext, v))
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

//act as dict with {} or {key:value}
bool PairOp::CurlyBracketRun(XlangRuntime* rt, XObj* pContext, Value& v, LValue* lValue)
{
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
			ExpExec(keyExpr,rt,action, pContext, retVal);
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
			Key = KeyProc((static_cast<Param*>(i))->GetName());
			ExpExec((static_cast<Param*>(i))->GetType(),rt,action, pContext, Val);
			break;
		case ObType::Assign:
		{
			auto* l = (static_cast<Assign*>(i))->GetL();
			ExpExec(l, rt, action, pContext, Key);
			ExpExec(l,rt, action, pContext, Val);
		}
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
		bool isDict = false;
		for (auto& i : list)
		{
			isDict = FindDict(i);
			if (isDict)
				break;
		}
		Data::Dict* pDict = nullptr;
		Data::mSet* pSet = nullptr;
		if (isDict)
		{
			pDict = new Data::Dict();
			v = Value(pDict);
		}
		else 
		{
			pSet = new Data::mSet();
			v = Value(pSet);
		}
		for (auto& i : list)
		{
			if (isDict)
			{
				SetKWProcDict(i,pDict);
			}
			else 
			{
				Value Val;
				ExecAction action;
				ExpExec(i,rt,action, pContext, Val);
				pSet->Set(Val);
			}
		}
	}
	else if(R)
	{
		bool isDict = FindDict(R);
		if (isDict)
		{
			auto* pDict = new Data::Dict();
			SetKWProcDict(R,pDict);
			v = Value(pDict);
		}
		else 
		{
			auto* pSet = new Data::mSet();
			Value Val;
			ExecAction action;
			ExpExec(R,rt,action, pContext, Val);
			pSet->Set(Val);
			v = Value(pSet);
		}
	}
	else
	{
		//for case {} act as dict
		v = X::Dict();
	}
	return true;
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
				ExpExec(r,rt,action, pContext, valDefaultValue);
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
bool PairOp::Set(XlangRuntime* rt, XObj* pContext, Value& v)
{
	Value leftObj;
	ExecAction action;
	bool bOK = ExpExec(L,rt, action, pContext, leftObj);
	if (!bOK || !leftObj.IsObject())
	{
		return false;
	}
	Value varIdx;
	bOK = ExpExec(R,rt, action, pContext, varIdx);
	if (!bOK)
	{
		return false;
	}
	X::Data::Object* pObj = dynamic_cast<X::Data::Object*>(leftObj.GetObj());
	pObj->Set(varIdx, v);
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