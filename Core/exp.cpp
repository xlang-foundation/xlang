#include "exp.h"
#include "builtin.h"
#include <iostream>
#include "object.h"
#include "runtime.h"

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

bool UnaryOp::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	Value v_r;
	if (!R->Run(rt,pContext,v_r))
	{
		return false;
	}
	auto func = G::I().OpAct(Op).unaryop;
	return func ? func(rt,this, v_r, v) : false;
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
bool Func::Run(Runtime* rt,void* pContext,Value& v, LValue* lValue)
{
	Data::Function* f = new Data::Function(this);
	Value v0(f);
	m_scope->Set(rt,pContext,m_Index, v0);
	v = v0;
	return true;
}

bool Func::Call(Runtime* rt,
	void* pContext, 
	std::vector<Value>& params,
	std::unordered_map<std::string, AST::Value>& kwParams,
	Value& retValue)
{
	static std::string THIS("this");
	auto* pContextObj = (X::Data::Object*)pContext;
	StackFrame* frame = new StackFrame(this);
	PushFrame(rt,frame);
	//Add this if This is not null
	int pre_item = 0;
	if (pContextObj && pContextObj->GetType() == X::Data::Type::XClassObject)
	{
		int thisIdx = AddOrGet(THIS, true);
		Value v0(pContext);
		Set(rt,pContext,thisIdx, v0);
		pre_item++;
	}
	int num = m_positionParamCnt > (int)params.size() ?
		(int)params.size() : m_positionParamCnt;
	for (int i = 0; i < num; i++)
	{
		Set(rt,pContext,pre_item+i, params[i]);
	}

	Value v0;
	Block::Run(rt, pContext,v0);
	PopFrame(rt);
	retValue = frame->GetReturnValue();
	delete frame;
	return true;
}
bool PairOp::GetParamList(Runtime* rt,Expression* e,
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
			bOK = valExpr->Run(rt,nullptr,v0);
			if (bOK)
			{
				kwParams.emplace(std::make_pair(strVarName, v0));
			}
		}
		else
		{
			Value v0;
			bOK = i->Run(rt,nullptr,v0);
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
bool PairOp::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	bool bOK = false;
	if (Op == G::I().GetOpId(OP_ID::Parenthesis_L))
	{	
		if (L)
		{//Call Func
			Value lVal;
			bOK = L->Run(rt,pContext,lVal, lValue);
			if (!bOK || !lVal.IsObject())
			{
				return bOK;
			}
			std::vector<Value> params;
			std::unordered_map<std::string, Value> kwParams;
			if (R)
			{
				bOK = GetParamList(rt,R, params, kwParams);
				if (!bOK)
				{
					return bOK;
				}
			}
			Data::Object* obj = (Data::Object*)lVal.GetObject();
			if (obj)
			{
				bOK = obj->Call(rt,params, kwParams, v);
			}
		}
		else
		{
			if (R && R->m_type != ObType::List)
			{
				bOK = R->Run(rt,pContext,v, lValue);
			}
		}
	}
	else if (Op == G::I().GetOpId(OP_ID::Brackets_L))
	{
		if (L)
		{//usage: x[1,2]
			Value v0;
			bOK = L->Run(rt,pContext,v0);
			Data::List* pDataList = (Data::List*)v0.GetObject();
			//Get Index
			std::vector<long long> IdxAry;
			if (R->m_type == ObType::List)
			{
				auto& list = ((List*)R)->GetList();
				for (auto e : list)
				{
					Value v1;
					if (e->Run(rt,pContext,v1))
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
				bOK = R->Run(rt,pContext,vIdx);
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
					if (e->Run(rt,pContext,v))
					{
						pDataList->Add(rt,v);
					}
					else
					{
						bOK = false;
						break;
					}
				}
			}
			else if(R && R->m_type == ObType::Var)
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
bool Block::Run(Runtime* rt,void* pContext, Value& v, LValue* lValue)
{
	bool bOk = true;
	for (auto i : Body)
	{
		Value v0;
		bOk = i->Run(rt,pContext, v0);
		if (!bOk)
		{
			break;
		}
		if (rt->M()->GetDbg() == dbg::Step)
		{
			std::cout << v0.ToString() << std::endl;
			int line = i->GetStartLine();
			std::cout << "(" << line << ",(c)ontinue,(s)tep)>>";
			X::AST::Value v0;
			std::string yes;
			std::cin >> yes;
			if (yes == "c" || yes == "C")
			{
				rt->M()->SetDbg(AST::dbg::Continue);
			}
			else if (yes == "s" || yes == "S")
			{
				rt->M()->SetDbg(AST::dbg::Step);
			}
		}
	}
	return bOk;
}
bool While::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	if (R == nil)
	{
		return false;
	}
	Value v0;
	while (true)
	{
		Value v0;
		bool bOK = R->Run(rt,pContext,v0);
		if (bOK && v0 == Value(true))
		{
			Block::Run(rt,pContext,v);
		}
		else
		{
			break;
		}
	}
	return true;
}
bool For::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	Value v0;
	while (true)
	{
		bool bC0 = R->Run(rt,pContext,v0);
		if (!bC0)
		{
			break;
		}
		Block::Run(rt,pContext,v);
	}
	return true;
}

bool Range::Eval(Runtime* rt)
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
				param->Run(rt,nullptr,vStop);
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
bool Range::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	if (!m_evaluated)
	{
		Eval(rt);
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
bool If::Run(Runtime* rt,void* pContext,Value& v,LValue* lValue)
{
	bool bRet = true;
	bool bCanRun = false;
	if (R)
	{
		Value v0;
		bool bOK = R->Run(rt,pContext,v0);
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
		bRet = Block::Run(rt,pContext,v);
	}
	else if(m_next)
	{
		bRet = m_next->Run(rt,pContext,v);
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
void Module::AddBuiltins(Runtime* rt)
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
			Set(rt,nullptr,idx, v0);
		}
	}
}
bool Assign::AssignToDataObject(Runtime* rt, void* pObjPtr)
{
	Data::Object* pObj = (Data::Object*)pObjPtr;
	Value v_r;
	if (!R->Run(rt,nullptr,v_r))
	{
		return false;
	}
	if (pObj->GetType() == Data::Type::FuncCalls)
	{
		Data::FuncCalls* pCalls = dynamic_cast<Data::FuncCalls*>(pObj);
		return pCalls->SetValue(v_r);
	}
	else
	{
		return false;
	}
}

void DotOp::ScopeLayout()
{
	if (L) L->ScopeLayout();
	//R will be decided in run stage
}
void DotOp::QueryBases(Runtime* rt,void* pObj0,std::vector<Expression*>& bases)
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
	else if (pObj->GetType() == Data::Type::Function ||
		pObj->GetType() == Data::Type::FuncCalls)
	{
		//for function, meta function like taskrun,
		//put into top module
		bases.push_back(rt->M());
	}
}
void DotOp::RunScopeLayoutWithScopes(Expression* pExpr, std::vector<Expression*>& scopes)
{
	if (pExpr->m_type == ObType::Pair)
	{
		PairOp* pPair = (PairOp*)pExpr;
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
bool DotOp::DotProcess(Runtime* rt, void* pContext, 
	Value& v_l, Expression* R,
	Value& v, LValue* lValue)
{
	std::vector<AST::Expression*> scopes;
	void* pLeftObj0 = v_l.GetObject();
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
		Data::ContextType conType,
		void* pContext)
	{
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
						if (pCallList == nil) pCallList = new Data::FuncCalls();
						pCallList->Add(conType, pContext, func, nil);
					}
				}
			}
			else
			{
				if (pValueList == nil) pValueList = new Data::List();
				pValueList->Add(lVal);
			}
		}
		else
		{
			if (pValueList == nil) pValueList = new Data::List();
			pValueList->Add(lVal);
		}
	};
	auto RunCallPerObj = [&](
		Expression* pExpr,
		Data::ContextType conType,
		void* pContext)
	{
		if (pExpr->m_type == ObType::Pair)
		{
			PairOp* pPair = (PairOp*)pExpr;
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
						AddFunc(v0, lVal, conType, pContext);
					}
				}
			}
			else if (pair_r && pair_r->m_type == ObType::Var)
			{
				Var* var = dynamic_cast<Var*>(pair_r);
				Value v0;
				LValue lVal = nil;
				var->Run(rt, pContext, v0, &lVal);
				AddFunc(v0, lVal, conType, pContext);
			}
		}
		else if (pExpr->m_type == ObType::Var)
		{
			Var* var = dynamic_cast<Var*>(pExpr);
			Value v0;
			LValue lValue = nil;
			var->Run(rt, pContext, v0, &lValue);
			AddFunc(v0, lValue, conType, pContext);
		}
	};
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
						Data::Object* pItObj = (Data::Object*)it.GetObject();
						if (pItObj->GetType() == Data::Type::XClassObject)
						{
							RunCallPerObj(R, Data::ContextType::Class, pItObj);
						}
						else if (pItObj->GetType() == Data::Type::Function)
						{
							RunCallPerObj(R, Data::ContextType::Class, pItObj);
						}
					}
				}
			}
		}
		else if (pLeftObj->GetType() == Data::Type::XClassObject)
		{
			RunCallPerObj(R, Data::ContextType::Class, pLeftObj);
		}
		else if (pLeftObj->GetType() == Data::Type::Function)
		{
			RunCallPerObj(R, Data::ContextType::Func, pLeftObj);
		}
		else if (pLeftObj->GetType() == Data::Type::FuncCalls)
		{
			RunCallPerObj(R, Data::ContextType::FuncCalls, pLeftObj);
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
bool DotOp::Run(Runtime* rt,void* pContext,Value& v, LValue* lValue)
{
	if (!L || !R)
	{
		return false;
	}
	Value v_l;
	if (!L->Run(rt, pContext, v_l) || !v_l.IsObject())
	{
		return false;
	}
	Expression* r = R;
	while (r->m_type == ObType::Dot)
	{
		DotOp* dotR = (DotOp*)r;
		Value v0;
		LValue lValue0=nil;
		DotProcess(rt, pContext, v_l, dotR->GetL(),v0,&lValue0);
		v_l = v0;
		r = dotR->GetR();
	}
	DotProcess(rt, pContext,v_l, r,v, lValue);
	return true;
}
bool XClass::Call(Runtime* rt,
	void* pContext,
	std::vector<Value>& params, 
	std::unordered_map<std::string, AST::Value>& kwParams,
	Value& retValue)
{
	Data::XClassObject* obj = new Data::XClassObject(this);
	if (m_constructor)
	{
		m_constructor->Call(rt,obj,params, kwParams,retValue);
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
XClass* XClass::FindBase(Runtime* rt, std::string& strName)
{
	XClass* pBase = nullptr;
	auto* pScope = GetScope();
	while (pScope != nullptr)
	{
		int idx = pScope->AddOrGet(strName, true);
		if (idx != -1)
		{
			Value v0;
			if (pScope->Get(rt,nullptr,idx, v0))
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
bool XClass::Set(Runtime* rt, void* pContext, int idx, Value& v)
{
	if (pContext)
	{
		Data::XClassObject* pObj = (Data::XClassObject*)pContext;
		pObj->GetStack()->Set(idx, v);
	}
	else if (m_stackFrame)
	{
		m_stackFrame->Set(idx, v);
	}
	return true;
}
bool XClass::Get(Runtime* rt,void* pContext, int idx, Value& v, LValue* lValue)
{
	if (pContext)
	{
		Data::XClassObject* pObj = (Data::XClassObject*)pContext;
		pObj->GetStack()->Get(idx, v, lValue);
	}
	else if (m_stackFrame)
	{
		m_stackFrame->Get(idx, v, lValue);
	}
	return true;
}
bool XClass::Run(Runtime* rt,void* pContext,Value& v, LValue* lValue)
{
	m_stackFrame = new StackFrame(this);
	m_stackFrame->SetVarCount((int)m_Vars.size());
	for (auto it : m_tempMemberList)
	{
		Value& v0 = it.second;
		bool bSet = false;
		if (v0.IsObject())
		{
			Data::Object* pObj = (Data::Object*)v0.GetObject();
			if (pObj->GetType() == Data::Type::Expr)
			{
				Data::Expr* pExpr = dynamic_cast<Data::Expr*>(pObj);
				if (pExpr)
				{
					AST::Expression*  pExpression = pExpr->Get();
					if (pExpression)
					{
						Value v1;
						if (pExpression->Run(rt,pContext,v1))
						{
							Set(rt,pContext,it.first, v1);
							bSet = true;
						}
					}
				}
			}
		}
		if (!bSet)
		{
			Set(rt,pContext,it.first, v0);
		}
	}
	m_tempMemberList.clear();

	Data::XClassObject* cls = new Data::XClassObject(this);
	Value v0(cls);
	bool bOK = m_scope->Set(rt,pContext,m_Index, v0);
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
				auto* pBaseClass = FindBase(rt,strVarName);
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