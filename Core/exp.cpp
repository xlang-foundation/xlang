#include "exp.h"
#include "builtin.h"
#include <iostream>
#include "object.h"

namespace X {namespace AST {
	Scope* Expression::FindScope()
{
	Scope* pMyScope = nil;
	Expression* pa = m_parent;
	while (pa != nil && pMyScope == nil)
	{
		pMyScope = dynamic_cast<Scope*>(pa);
		pa = pa->GetParent();
	}
	return pMyScope;
}

Func* Expression::FindFuncByName(Var* name)
{
	Func* pFuncRet = nil;
	Expression* pa = m_parent;
	while (pa != nil)
	{
		Block* pMyBlock = dynamic_cast<AST::Block*>(pa);
		if (pMyBlock)
		{
			pFuncRet = pMyBlock->FindFuncByName(name);
			if (pFuncRet)
			{
				break;
			}
		}
		pa = pa->GetParent();
	}
	return pFuncRet;
}

bool UnaryOp::Run(Value& v,LValue* lValue)
{
	Value v_r;
	if (!R->Run(v_r))
	{
		return false;
	}
	auto func = G::I().OpAct(Op).unaryop;
	return func ? func(this, v_r, v) : false;
}
void Func::ScopeLayout()
{
	static std::string THIS("this");
	Scope* pMyScope = GetScope();
	if (pMyScope)
	{
		std::string strName(m_Name.s, m_Name.size);
		m_Index = pMyScope->AddOrGet(strName, false);
		if (pMyScope->m_type == ObType::Class)
		{//it is class's member
			AddOrGet(THIS, false);
		}
	}
	//prcoess parameters' default values
	if (Params)
	{
		auto& list = Params->GetList();
		m_positionParamCnt = (int)list.size();
		for (auto i : list)
		{
			std::string strVarName;
			std::string strVarType;
			Value defaultValue;
			switch (i->m_type)
			{
			case ObType::Var:
			{
				Var* varName = dynamic_cast<Var*>(i);
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
			}
			break;
			case ObType::Assign:
			{
				Assign* assign = dynamic_cast<Assign*>(i);
				Var* varName = dynamic_cast<Var*>(assign->GetL());
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				Expression* defVal = assign->GetR();
				auto* pExprForDefVal = new Data::Expr(defVal);
				defaultValue = Value(pExprForDefVal);
			}
			break;
			case ObType::Param:
			{ 
				Param* param = dynamic_cast<Param*>(i);
				param->Parse(strVarName, strVarType, defaultValue);
			}
			break;
			}
			AddOrGet(strVarName, false);
		}
	}
}
bool Func::Run(Value& v, LValue* lValue)
{
	Data::Function* f = new Data::Function(this);
	Value v0(f);
	m_scope->Set(m_Index, v0);
	v = v0;
	return true;
}

bool Func::Call(void* This, std::vector<Value>& params, Value& retValue)
{
	static std::string THIS("this");
	StackFrame* frame = new StackFrame();
	PushFrame(frame);
	//Add this if This is not null
	int pre_item = 0;
	if (This)
	{
		int thisIdx = AddOrGet(THIS, true);
		Value v0(This);
		Set(thisIdx, v0);
		pre_item++;
	}
	int num = m_positionParamCnt > (int)params.size() ?
		(int)params.size() : m_positionParamCnt;
	for (int i = 0; i < num; i++)
	{
		Set(pre_item+i, params[i]);
	}

	Value v0;
	Block::Run(v0);
	PopFrame();
	retValue = frame->GetReturnValue();
	delete frame;
	return true;
}
bool PairOp::GetParamList(Expression* e,
	std::vector<Value>& params,
	std::unordered_map<std::string, Value>& kwParams)
{
	auto proc = [&](Expression* i)
	{
		bool bOK = true;
		if (i->m_type == ObType::Assign)
		{
			Assign* assign = dynamic_cast<Assign*>(i);
			Var* varName = dynamic_cast<Var*>(assign->GetL());
			String& szName = varName->GetName();
			std::string strVarName = std::string(szName.s, szName.size);
			Expression* valExpr = assign->GetR();
			Value v0;
			bOK = valExpr->Run(v0);
			if (bOK)
			{
				kwParams.emplace(std::make_pair(strVarName, v0));
			}
		}
		else
		{
			Value v0;
			bOK = i->Run(v0);
			if (bOK)
			{
				params.push_back(v0);
			}
		}
		return bOK;
	};
	bool bOK = true;
	if (e->m_type != ObType::List)
	{
		bOK = proc(e);
	}
	else
	{
		auto& list = ((List*)e)->GetList();
		for (auto i : list)
		{
			bOK = proc(i);
			if (!bOK)
			{
				break;
			}
		}
	}
	return bOK;
}
bool PairOp::Run(Value& v,LValue* lValue)
{
	bool bOK = false;
	if (Op == G::I().GetOpId(OP_ID::Parenthesis_L))
	{	
		if (L)
		{//Call Func
			Value lVal;
			bOK = L->Run(lVal, lValue);
			if (!bOK || !lVal.IsObject())
			{
				return bOK;
			}
			std::vector<Value> params;
			std::unordered_map<std::string, Value> kwParams;
			if (R)
			{
				bOK = GetParamList(R, params, kwParams);
				if (!bOK)
				{
					return bOK;
				}
			}
			Data::Object* obj = (Data::Object*)lVal.GetObject();
			if (obj)
			{
				bOK = obj->Call(params, kwParams, v);
			}
		}
		else
		{
			if (R && R->m_type != ObType::List)
			{
				bOK = R->Run(v, lValue);
			}
		}
	}
	else if (Op == G::I().GetOpId(OP_ID::Brackets_L))
	{
		if (L)
		{//usage: x[1,2]
			Value v0;
			bOK = L->Run(v0);
			Data::List* pDataList = (Data::List*)v0.GetObject();
			//Get Index
			std::vector<long long> IdxAry;
			if (R->m_type == ObType::List)
			{
				auto& list = ((List*)R)->GetList();
				for (auto e : list)
				{
					Value v1;
					if (e->Run(v1))
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
				bOK = R->Run(vIdx);
				IdxAry.push_back(vIdx.GetLongLong());
			}
			if (bOK)
			{
				if (IdxAry.size() > 0)
				{
					pDataList->Get(IdxAry[0],v, lValue);
				}
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
					if (e->Run(v))
					{
						pDataList->Add(v);
					}
					else
					{
						bOK = false;
						break;
					}
				}
			}
			v = Value(pDataList);
		}
	}
	return bOK;
}
void Block::Add(Expression* item)
{
	int line = item->GetStartLine();
	if (Body.size() > 0)
	{//for if elif else 
		Expression* pLastExp = Body[Body.size() - 1];
		if (pLastExp->EatMe(item))
		{
			item->ScopeLayout();
			return;
		}
	}
	Body.push_back(item);
	item->SetParent(this);
	item->ScopeLayout();

}
Func* Block::FindFuncByName(Var* name)
{
	Func* func = nil;
	String& target_name = name->GetName();
	//internl first
	std::string strName(target_name.s, target_name.size);
	ExternFunc* extFunc = Builtin::I().Find(strName);
	if (extFunc)
	{
		return extFunc;
	}
	for (auto i : Body)
	{
		if (i->m_type != ObType::Func)
		{
			continue;
		}
		auto iFunc = (Func*)i;
		Var* ivarName = dynamic_cast<Var*>(iFunc->GetName());
		if (ivarName == nil)
		{
			continue;
		}
		String& i_name = ivarName->GetName();
		bool bMatched = false;
		if (i_name.size == target_name.size && i_name.size>0)
		{
			bMatched = true;
			for (int j = 0; j < i_name.size; j++)
			{
				if (i_name.s[j] != target_name.s[j])
				{
					bMatched = false;
					break;
				}
			}
		}
		if (bMatched)
		{
			func = iFunc;
			break;
		}
	}
	return func;
}

bool While::Run(Value& v,LValue* lValue)
{
	if (R == nil)
	{
		return false;
	}
	Value v0;
	while (true)
	{
		Value v0;
		bool bOK = R->Run(v0);
		if (bOK && v0 == Value(true))
		{
			Block::Run(v);
		}
		else
		{
			break;
		}
	}
	return true;
}
bool For::Run(Value& v,LValue* lValue)
{
	Value v0;
	while (true)
	{
		bool bC0 = R->Run(v0);
		if (!bC0)
		{
			break;
		}
		Block::Run(v);
	}
	return true;
}

#if 0
bool InOp::Run(Value& v,LValue* lValue)
{
	bool bIn = false;
	if (R)
	{
		if (R->m_type == ObType::Range)
		{
			Range* r = dynamic_cast<Range*>(R);
			bIn = r->Run(v);//will update v
			if (bIn)
			{
				L->Set(v);
			}
		}
	}
	return bIn;
}
#endif
bool Range::Eval()
{
	if (R && R->m_type == ObType::Pair)
	{
		PairOp* p = dynamic_cast<PairOp*>(R);
		Expression* param = p->GetR();
		if (param)
		{
			if (param->m_type == ObType::List)
			{

			}
			else
			{//only one parameter, means stop
				Value vStop;
				param->Run(vStop);
				if (vStop.GetType() == ValueType::Int64)
				{
					m_stop = vStop.GetLongLong();
				}
			}
		}
		
	}
	m_evaluated = true;
	return true;
}
bool Range::Run(Value& v,LValue* lValue)
{
	if (!m_evaluated)
	{
		Eval();
	}
	if (v.GetType() != ValueType::Int64)
	{//not started
		v = Value(m_start);
	}
	else
	{
		v += m_step;
	}
	return (v.GetLongLong()<m_stop);
}
bool If::EatMe(Expression* other)
{
	If* elseIf = dynamic_cast<If*>(other);
	if (elseIf)
	{
		If* p = this;
		If* n = m_next;
		while (n != nil)
		{
			p = n;
			n = n->m_next;
		}
		p->m_next = elseIf;
		elseIf->SetParent(p);
		return true;
	}
	else
	{
		return false;
	}
}
bool If::Run(Value& v,LValue* lValue)
{
	bool bRet = true;
	bool bCanRun = false;
	if (R)
	{
		Value v0;
		bool bOK = R->Run(v0);
		if (bOK && v0 == Value(true))
		{
			bCanRun = true;
		}
	}
	else
	{//for Else in if 
		bCanRun = true;
	}
	if (bCanRun)
	{
		bRet = Block::Run(v);
	}
	else if(m_next)
	{
		bRet = m_next->Run(v);
	}
	return bRet;
}

void ColonOP::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	auto operandR = operands.top();
	operands.pop();
	auto operandL = operands.top();
	operands.pop();
	auto param = new AST::Param(operandL, operandR);
	operands.push(param);
}

void CommaOp::OpWithOperands(std::stack<AST::Expression*>& operands)
{
	auto operandR = operands.top();
	operands.pop();
	auto operandL = operands.top();
	operands.pop();
	AST::List* list = nil;
	if (operandL->m_type != AST::ObType::List)
	{
		list = new AST::List(operandL);
	}
	else
	{
		list = (AST::List*)operandL;
	}
	if (operandR->m_type != AST::ObType::List)
	{
		*list += operandR;
	}
	else
	{
		List& list_r = *(AST::List*)operandR;
		*list += list_r;
		list_r.ClearList();
		delete operandR;
	}
	operands.push(list);

}
void Module::ScopeLayout()
{
	auto& funcs = Builtin::I().All();
	for (auto it : funcs)
	{
		auto name = it.first;
		int idx = AddOrGet(name, false);
	}
	Scope::ScopeLayout();
}
void Module::AddBuiltins()
{
	auto& funcs = Builtin::I().All();
	for (auto it : funcs)
	{
		auto name = it.first;
		int idx = AddOrGet(name, true);
		if (idx >= 0)
		{
			auto* pFuncObj = new Data::Function(it.second);
			Value v0(pFuncObj);
			Set(idx, v0);
		}
	}
}
void DotOp::ScopeLayout()
{
	if (L) L->ScopeLayout();
	//R will be decided in run stage
}
void DotOp::QueryBases(void* pObj0,std::vector<Expression*>& bases)
{
	Data::Object* pObj = (Data::Object*)pObj0;
	if (pObj->GetType() == Data::Type::List)
	{
		Data::List* pList = dynamic_cast<Data::List*>(pObj);
		if (pList)
		{
			auto& bs = pList->GetBases();
			for (auto it : bs)
			{
				bases.push_back(it);
			}
		}
	}
	else if (pObj->GetType() == Data::Type::XClassObject)
	{
		Data::XClassObject* pClassObj = dynamic_cast<Data::XClassObject*>(pObj);
		if (pClassObj)
		{
			bases.push_back(pClassObj->GetClassObj());
		}
	}
	else
	{//TODO:

	}
}
void DotOp::RunScopeLayoutWithScopes(Expression* pExpr, std::vector<Expression*>& scopes)
{
	if (pExpr->m_type == ObType::Pair)
	{
		AST::List* pList = dynamic_cast<AST::List*>(((PairOp*)pExpr)->GetR());
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
	else if (pExpr->m_type == ObType::Var)
	{
		Var* var = dynamic_cast<Var*>(R);
		var->ScopeLayout(scopes);
	}
}
bool DotOp::Run(Value& v, LValue* lValue)
{
	if (!L || !R)
	{
		return false;
	}
	Value v_l;
	if (!L->Run(v_l) || !v_l.IsObject())
	{
		return false;
	}
	std::vector<AST::Expression*> scopes;
	void* pLeftObj0 = v_l.GetObject();
	if (pLeftObj0)
	{
		QueryBases(pLeftObj0, scopes);
	}
	//R can be a Var or List
	if (R)
	{
		RunScopeLayoutWithScopes(R, scopes);
	}

	auto RunCallPerObj = [](
		Data::FuncCalls* pCalls,
		Expression* pExpr,
		Data::XClassObject* pClassObj)
	{
		if (pExpr->m_type == ObType::Pair)
		{
			AST::List* pList = dynamic_cast<AST::List*>(((PairOp*)pExpr)->GetR());
			auto& list = pList->GetList();
			for (auto it : list)
			{
				if (it->m_type == ObType::Var)
				{
					Var* var = dynamic_cast<Var*>(it);
					Value v0;
					var->Run(v0);
					if (v0.IsObject())
					{
						Data::Object* pObj0 = (Data::Object*)v0.GetObject();
						if (pObj0 && pObj0->GetType() == Data::Type::Function)
						{
							Data::Function* pFuncObj = dynamic_cast<Data::Function*>(pObj0);
							if (pFuncObj)
							{
								Func* func = pFuncObj->GetFunc();
								if (func)
								{
									pCalls->Add(pClassObj, func);
								}
							}
						}
					}
				}
			}
		}
		else if (pExpr->m_type == ObType::Var)
		{
			Var* var = dynamic_cast<Var*>(pExpr);
			Value v0;
			var->Run(v0);
			if (v0.IsObject())
			{
				Data::Object* pObj0 = (Data::Object*)v0.GetObject();
				if (pObj0 && pObj0->GetType() == Data::Type::Function)
				{
					Data::Function* pFuncObj = dynamic_cast<Data::Function*>(pObj0);
					if (pFuncObj)
					{
						Func* func = pFuncObj->GetFunc();
						if (func)
						{
							pCalls->Add(pClassObj, func);
						}
					}
				}
			}
		}
	};
	Data::FuncCalls* pCallList = new Data::FuncCalls();
	if (pLeftObj0)
	{
		Data::Object* pLeftObj = (Data::Object*)pLeftObj0;
		if (pLeftObj->GetType() == Data::Type::List)
		{
			Data::List* pList = dynamic_cast<Data::List*>(pLeftObj);
			if (pList)
			{
				auto& data = pList->Data();
				for (auto& it : data)
				{
					if (it.IsObject())
					{
						Data::XClassObject* pClassObj = 
							dynamic_cast<Data::XClassObject*>((Data::Object*)it.GetObject());
						if (pClassObj)
						{
							RunCallPerObj(pCallList, R, pClassObj);
						}
					}
				}
			}
		}
		else if (pLeftObj->GetType() == Data::Type::XClassObject)
		{
			Data::XClassObject* pClassObj = dynamic_cast<Data::XClassObject*>(pLeftObj);
			if (pClassObj)
			{
				RunCallPerObj(pCallList, R, pClassObj);
			}
		}
	}
	v = Value(pCallList);
	return true;
}
bool XClass::Call(std::vector<Value>& params, Value& retValue)
{
	Data::XClassObject* obj = new Data::XClassObject(this);
	if (m_constructor)
	{
		m_constructor->Call(obj,params, retValue);
	}
	retValue = Value(obj);
	return true;
}
void XClass::ScopeLayout()
{
	Scope* pMyScope = GetScope();
	if (pMyScope)
	{
		std::string strName(m_Name.s, m_Name.size);
		m_Index = pMyScope->AddOrGet(strName, false);
	}
}
XClass* XClass::FindBase(std::string& strName)
{
	XClass* pBase = nullptr;
	auto* pScope = GetScope();
	while (pScope != nullptr)
	{
		int idx = pScope->AddOrGet(strName, true);
		if (idx != -1)
		{
			Value v0;
			if (pScope->Get(idx, v0))
			{
				if (v0.IsObject())
				{
					Data::Object* pBaseObj = (Data::Object*)v0.GetObject();
					auto* pXBaseObj = dynamic_cast<Data::XClassObject*>(pBaseObj);
					if (pXBaseObj)
					{
						pBase = pXBaseObj->GetClassObj();
						break;
					}
				}
			}
		}
		pScope = pScope->GetScope();
	}
	return pBase;
}
bool XClass::Run(Value& v, LValue* lValue)
{
	StackFrame* frame = new StackFrame();
	PushFrame(frame);//to hold properties and funcs
	for (auto it : m_tempMemberList)
	{
		Set(it.first, it.second);
	}
	m_tempMemberList.clear();

	Data::XClassObject* cls = new Data::XClassObject(this);
	Value v0(cls);
	bool bOK = m_scope->Set(m_Index, v0);
	v = v0;

	//prcoess base classes
	if (Params)
	{
		auto& list = Params->GetList();
		for (auto i : list)
		{
			std::string strVarName;
			if (i->m_type == ObType::Var)
			{
				Var* varName = dynamic_cast<Var*>(i);
				String& szName = varName->GetName();
				strVarName = std::string(szName.s, szName.size);
				auto* pBaseClass = FindBase(strVarName);
				if (pBaseClass)
				{
					m_bases.push_back(pBaseClass);
				}
			}
		}
	}
	return bOK;
}

void XClass::Add(Expression* item)
{
	switch (item->m_type)
	{
	case ObType::Param:
	{
		Param* param = dynamic_cast<Param*>(item);
		std::string strVarName;
		std::string strVarType;//TODO: deal with type
		Value defaultValue;
		if (param->Parse(strVarName, strVarType, defaultValue))
		{
			int idx = AddOrGet(strVarName,false);
			m_tempMemberList.push_back(std::make_pair(idx, defaultValue));
		}
	}
		break;
	case ObType::Func:
	{
		Func* func = dynamic_cast<Func*>(item);
		String& funcName = func->GetNameStr();
		std::string strName(funcName.s, funcName.size);
		int idx = AddOrGet(strName, false);
		Data::Function* f = new Data::Function(func);
		Value funcObj(f);
		m_tempMemberList.push_back(std::make_pair(idx, funcObj));
		if (strName == "constructor")//TODO: add class name also can be constructor
		{
			m_constructor = func;
		}
	}
		break;
	default:
		break;
	}

	item->SetParent(this);
	item->ScopeLayout();
}
bool Param::Parse(std::string& strVarName, 
	std::string& strVarType, Value& defaultValue)
{
	//two types: 1) name:type=val 2) name:type
	Var* varName = dynamic_cast<Var*>(GetName());
	String& szName = varName->GetName();
	strVarName = std::string(szName.s, szName.size);
	Expression* typeCombine = GetType();
	if (typeCombine->m_type == ObType::Assign)
	{
		Assign* assign = dynamic_cast<Assign*>(typeCombine);
		Var* type = dynamic_cast<Var*>(assign->GetL());
		if (type)
		{
			String& szName = type->GetName();
			strVarType = std::string(szName.s, szName.size);
		}
		Expression* defVal = assign->GetR();
		auto* pExprForDefVal = new Data::Expr(defVal);
		defaultValue = Value(pExprForDefVal);
	}
	else if (typeCombine->m_type == ObType::Var)
	{
		Var* type = dynamic_cast<Var*>(typeCombine);
		if (type)
		{
			String& szName = type->GetName();
			strVarType = std::string(szName.s, szName.size);
		}
	}
	return true;
}

}
}