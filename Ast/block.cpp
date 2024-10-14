/*
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

#include "block.h"
#include "var.h"
#include "func.h"
#include "builtin.h"
#include "module.h"
#include <iostream>
#include "utility.h"
#include "InlineCall.h"
#include "iterator.h"

extern bool U_Print(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue);

namespace X
{
namespace AST
{
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
	ReCalcHint(item);
	Body.push_back(item);
	item->SetParent(this);
	item->ScopeLayout();

}
bool Block::ExecForTrace(XlangRuntime* rt, ExecAction& action,XObj* pContext, Value& v, LValue* lValue)
{
	bool bOk = true;
	m_bRunning = true;
	auto Trace = G::I().GetTrace();
	Scope* pCurScope = nullptr;
	bool useMyScope = (m_type == ObType::Func 
		|| m_type == ObType::Class 
		|| m_type == ObType::Module);
	if (useMyScope)
	{
		pCurScope = GetMyScope();
	}
	else
	{
		pCurScope = GetScope();
	}

	//if being traced, go to here
	bool bEnterBlock = false;
	if (useMyScope && rt->GetDbgType() == X::dbg::StepIn)
	{
		bEnterBlock = true;
		Trace(rt, pContext,rt->GetCurrentStack(),TraceEvent::Call, pCurScope,nullptr); // do nothing
		//then we need to change DbgType to Step from StepIn
		//because the lines exec in Body will be step by step
		rt->SetDbgType(X::dbg::Step, X::dbg::StepIn); // 
	}
	auto last = Body[Body.size() - 1];
	for (auto& i : Body)
	{
		//Update Stack Frame
		int line = i->GetStartLine();
		int pos = i->GetCharPos();
		rt->GetCurrentStack()->SetLine(line);
		rt->GetCurrentStack()->SetCharPos(pos);
		rt->GetCurrentStack()->SetCurExp(i);
		//std::cout << "Run Line(before check):" << line <<std::endl;

		Trace(rt, pContext, rt->GetCurrentStack(),TraceEvent::Line, pCurScope, i);

		if (i->m_type == ObType::ActionOp)
		{
			auto* pActionOperator = dynamic_cast<ActionOperator*>(i);
			OP_ID opId = pActionOperator->GetId();
			switch (opId)
			{
			case OP_ID::Break:
				action.type = ExecActionType::Break;
				break;
			case OP_ID::Continue:
				action.type = ExecActionType::Continue;
				break;
			case OP_ID::Pass:
				continue;//just run next line					
			default:
				break;
			}
		}
		Value v0;
		ExecAction action0;
		bOk = ExpExec(i, rt, action0, pContext, v0, lValue);
		//if break or cotinue action passed back
		//break this loop,and pass back to caller
		if (action0.type == ExecActionType::Break ||
			action0.type == ExecActionType::Continue)
		{
			action = action0;
			break;
		}
		//std::cout << "after run line:" << line << std::endl;
		if (!bOk)
		{//TODO: error process here
			auto pid = GetPID();
			std::cout << "Error Occurs in line:" << line << ",pid:" << pid << std::endl;
			auto code = i->GetCode();
			std::cout <<"*** " << code << std::endl;
			//break;
		}
		if (v0.IsValid() && (i == last))
		{
			v = v0;
		}
		if (action0.type == ExecActionType::Return)
		{
			action = action0;
			break;
		}
	}
	m_bRunning = false;
	if (bEnterBlock && (m_type == ObType::Func || m_type == ObType::Module))
	{
		Trace(rt, pContext, rt->GetCurrentStack(),TraceEvent::Return, pCurScope,nullptr); //
		
	}
	if (useMyScope)
	{
		// change DbgType from StepOut to Step, if this block is the root function to step over or step out this block only(not step over a function or step over a function but interrupted by a breakpoint )
		if (rt->GetDbgType() == X::dbg::StepOut && (this == rt->m_pFirstStepOutExp || !rt->m_pFirstStepOutExp)) // 
		{
			rt->SetDbgType(dbg::Step, X::dbg::StepOut);
			rt->m_pFirstStepOutExp = nullptr;
		}
	}

	return bOk;
}

bool Block::RunFromLine(XRuntime* rt, XObj* pContext,
	long long lineNo, Value& v, LValue* lValue)
{
	auto print_var = [](XRuntime* rt, XObj* pContext,Expression* exp)
	{
		if (exp->m_type == ObType::Var)
		{
			//just print out
			Value v0;
			ExecAction action;
			bool bOK = ExpExec(exp,(XlangRuntime*)rt, action,pContext, v0);
			if (bOK)
			{
				X::ARGS args_p(1);
				args_p.push_back(v0);
				X::KWARGS kwArgs_p;
				X::Value retValue_p;
				U_Print(rt, nullptr,pContext, args_p, kwArgs_p, retValue_p);
			}
			return bOK;
		}
		else
		{
			return false;
		}
	};
	bool bOK = false;
	auto lineCnt = Body.size();
	for (long long i = lineNo; i < lineCnt; i++)
	{
		auto line = Body[i];
		if (line->m_type == ObType::Var)
		{
			print_var(rt, pContext, line);
			continue;
		}
		else if (line->m_type == ObType::List)
		{
			List* pList = dynamic_cast<List*>(line);
			auto& list = pList->GetList();
			for (auto& p : list)
			{
				print_var(rt, pContext, p);
			}
			continue;
		}
		Value v0;
		ExecAction action;
		bOK = ExpExec(line,(XlangRuntime*)rt,action,pContext, v0);
		if (!bOK)
		{
			break;
		}
		if (i == (lineCnt - 1))
		{
			v = v0;
		}
	}
	return bOK;
}
bool Block::RunLast(XRuntime* rt0, XObj* pContext, Value& v, LValue* lValue)
{
	if (Body.size() == 0)
	{
		return false;
	}
	auto last = Body[Body.size() - 1];
	Value v0;
	ExecAction action;
	bool bOk = ExpExec(last,(XlangRuntime*)rt0, action,pContext, v0);
	if (bOk)
	{
		v = v0;
	}
	return bOk;
}
bool While::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue)
{
	if (R == nil)
	{
		return false;
	}
	Value v0;
	while (true)
	{
		Value v0;
		ExecAction action;
		bool bOK = ExpExec(R,rt,action,pContext,v0);
		if (bOK && v0.IsTrue())
		{
			ExecAction action;
			Block::Exec(rt,action,pContext,v);
			if (action.type == ExecActionType::Break)
			{
				break;//break while
			}
		}
		else
		{
			break;
		}
	}
	return true;
}

void For::ScopeLayout()
{
	if (R->m_type != ObType::In)
	{
		return;
	}
	InOp* pIn = dynamic_cast<InOp*>(R);
	Expression* iterableExp = pIn->GetR();
	Expression* varExp = pIn->GetL();
	if (varExp == nullptr || iterableExp == nullptr)
	{
		return;
	}
	//change varExp's to left value
	varExp->SetIsLeftValue(true);

	varExp->ScopeLayout();
	iterableExp->ScopeLayout();
}

bool For::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue)
{
	//R must be InOp
	//so its left is a variable
	//right is iterable object for example:list,dict,range
	if (R->m_type != ObType::In)
	{
		return false;
	}
	InOp* pIn = dynamic_cast<InOp*>(R);
	Expression* iterableExp = pIn->GetR();
	Expression* varExp = pIn->GetL();
	if(varExp == nullptr || iterableExp == nullptr)
	{
		return false;
	}

	ExecAction action_it;
	X::Value iterableObj;
	LValue* lValue_it = nullptr;
	bool bOK = ExpExec(iterableExp, rt, action_it, pContext, iterableObj, lValue_it);
	if (!bOK)
	{
		return false;
	}
	auto* pDataObj = dynamic_cast<Data::Object*>(iterableObj.GetObj());
	if(!pDataObj)
	{
		return false;
	}
	X::Data::Iterator_Pos curPos = nullptr;
	std::vector<Value> vals;
	while (pDataObj->GetAndUpdatePos(curPos, vals, false))
	{
		varExp->SetArry(dynamic_cast<XlangRuntime*>(rt),pContext, vals);
		vals.clear();
		ExecAction action0;
		Block::Exec_i(rt, action0, pContext, v);
		//if break, will break this while loop
		//if continue, continue loop
		if (action0.type == ExecActionType::Break)
		{
			break;//break while
		}
		else if (action0.type == ExecActionType::Return)
		{
			action = action0;//need to pass back to up level if got return until it meet function
			break;
		}
	}
	return true;
}

bool If::EatMe(Expression* other)
{
	If* elseIf = dynamic_cast<If*>(other);
	//if it is elif or else, then eat by the previous one
	if (elseIf && !elseIf->IsIf())
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
bool If::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue)
{
	bool bRet = true;
	bool bCanRun = false;
	if (R)
	{
		Value v0;
		ExecAction actionR;
		bool bOK = ExpExec(R,rt, actionR,pContext,v0);
		if (bOK && v0.IsTrue())
		{
			bCanRun = true;
		}
	}
	else
	{//for Else in if 
		bCanRun = true;
	}
	//if or else if will pass action back to caller
	//for example, caller is for or while
	//will take this action
	if (bCanRun)
	{
		bRet = Block::Exec_i(rt,action,pContext,v);
	}
	else if(m_next)
	{
		bRet = ExpExec(m_next,rt,action,pContext,v);
	}
	return bRet;
}
}
}