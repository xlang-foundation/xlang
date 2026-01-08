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

#include "exp.h"
#include "builtin.h"
#include <iostream>
#include "object.h"
#include "runtime.h"
#include "../Parse/parser.h"
#include "op.h"
#include "var.h"
#include "block.h"
#include "func.h"
#include "str.h"
#include "utility.h"
#include "pipeop.h"
#include "import.h"
#include "module.h"
#include "complex.h"
#include "list.h"
#include "inline_expr.h"
namespace X 
{
namespace AST 
{
std::string Expression::GetCode()
{
	Module* pMyModule = nil;
	Expression* pa = m_parent;
	while (pa != nil)
	{	
		if (pa->m_type == ObType::Module)
		{
			pMyModule = dynamic_cast<Module*>(pa);
			break;
		}
		pa = pa->GetParent();
	}
	if (pMyModule)
	{
		return pMyModule->GetCodeFragment(m_charStart, m_charEnd);
	}
	else
	{
		return "";
	}
}
Expression::~Expression()
{
	if(m_pMyScope)
	{
		delete m_pMyScope;
		m_pMyScope = nullptr;
	}
}
Expression *Expression::CreateByType(ObType t)
{
	Expression* pExp = nullptr;
	switch (t)
	{
	case ObType::Base:
		pExp = new Expression();
		break;
	case ObType::Assign:
		pExp = new Assign();
		break;
	case ObType::BinaryOp:
		pExp = new BinaryOp();
		break;
	case ObType::UnaryOp:
		pExp = new UnaryOp();
		break;
	case ObType::PipeOp:
		pExp = new PipeOp();
		break;
	case ObType::In:
		pExp = new InOp();
		break;
	case ObType::Var:
		pExp = new Var();
		break;
	case ObType::Str:
		pExp = new Str();
		break;
	case ObType::Const:
		pExp = new XConst();
		break;
	case ObType::Number:
		pExp = new Number();
		break;
	case ObType::Double:
		pExp = new Double();
		break;
	case ObType::Param:
		pExp = new Param();
		break;
	case ObType::List:
		pExp = new List();
		break;
	case ObType::Pair:
		pExp = new PairOp();
		break;
	case ObType::Dot:
		pExp = new DotOp();
		break;
	case ObType::Decor:
		pExp = new Decorator();
		break;
	case ObType::Func:
		pExp = new Func();
		break;
	case ObType::BuiltinFunc:
		pExp = new ExternFunc();
		break;
	case ObType::Module:
		pExp = new Func();
		break;
	case ObType::Block:
		pExp = new Block();
		break;
	case ObType::Class:
		pExp = new XClass();
		break;
	case ObType::ActionOp:
		pExp = new ActionOperator();
		break;
	case ObType::From:
		pExp = new From();
		break;
	case ObType::ColonOp:
		pExp = new ColonOP();
		break;
	case ObType::CommaOp:
		pExp = new CommaOp();
		break;
	case ObType::SemicolonOp:
		pExp = new SemicolonOp();
		break;
	case ObType::As:
		pExp = new AsOp();
		break;
	case ObType::For:
		pExp = new For();
		break;
	case ObType::While:
		pExp = new While();
		break;
	case ObType::If:
		pExp = new If();
		break;
	case ObType::ExternDecl:
		pExp = new ExternDecl();
		break;
	case ObType::Thru:
		pExp = new ThruOp();
		break;
	case ObType::Import:
		pExp = new Import();
		break;
	case ObType::TernaryOp:
		pExp = new TernaryOp();
		break;
	case ObType::ListComprehension:
		pExp = new ListComprehension();
		break;
	case ObType::DictComprehension:
		pExp = new DictComprehension();
		break;
	case ObType::InlineIfOp:
		pExp = new InlineIfOp();
		break;
	case ObType::InlineElseOp:
		pExp = new InlineElseOp();
		break;
	default:
		break;
	}
	if (pExp)
	{
		pExp->m_type = t;
	}
	return pExp;
}
bool Expression::ToBytes(XlangRuntime* rt, XObj* pContext,X::XLangStream& stream)
{
	stream << m_type;
	ExpId parentId = 0;
	if (m_parent) parentId = m_parent->ID();
	ExpId scopeId = 0;
	if (m_scope) scopeId = m_scope->ID();
	stream << parentId;
	stream << scopeId;
	stream << m_isLeftValue;
	stream << m_lineStart;
	stream << m_lineEnd;
	stream << m_charPos;
	return true;
}
bool Expression::FromBytes(X::XLangStream& stream)
{
	//m_type already loaded in callee
	ExpId parentId = 0;
	ExpId scopeId = 0;
	stream >> parentId;
	stream >> scopeId;
	//will look back to get the real addres
	m_parent = (Expression*)stream.ScopeSpace().Query(parentId);
	m_scope = (Scope*)stream.ScopeSpace().Query(scopeId);

	stream >> m_isLeftValue;
	stream >> m_lineStart;
	stream >> m_lineEnd;
	stream >> m_charPos;
	return true;
}
FORCE_INLINE bool Param::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue)
{
	bool bOK = true;
	if (Name)
	{
		//if it is left value, for example inside a class
		//as class's attribute.
		if (Name->IsLeftValue())
		{
			//we get this case inside a class
			// var_name:Type = value
			//and we will get var_name as Name
			//but we get Type = value as assign expression as Type
			//so we will use same logic but with changing Type'L with Name
			if (Type)
			{
				if (Type->m_type == ObType::Assign)
				{
					auto* pTypeAssign = dynamic_cast<Assign*>(Type);
					auto* L_keep = pTypeAssign->GetL();
					pTypeAssign->SetL(Name);
					bOK = ExpExec(pTypeAssign,rt, action, pContext, v, lValue);
					//restore back
					pTypeAssign->SetL(L_keep);
				}
				else
				{
					bOK = ExpExec(Name, rt, action, pContext, v, lValue);
				}
			}
			else
			{
				bOK = ExpExec(Name,rt, action, pContext, v, lValue);
			}
		}
		else
		{
			bOK = ExpExec(Name,rt, action, pContext, v);
		}
	}
	return bOK;
}
FORCE_INLINE bool Param::Parse(std::string& strVarName, 
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


bool Expression::RunStringExpWithFormat(XlangRuntime* rt, XObj* pContext,
	const char* s_in, int size, std::string& outStr, bool isFString, bool UseBindMode,
	std::vector<X::Value>& bind_data_list)
{
	std::string& str0 = outStr;
	bool bMeetSlash = false;
	bool IsOctal = false;
	int ecsVal = 0;
	int digitCount = 0;
	bool IsHex = false;
	int i = 0;
	//std::cout << "DEBUG: i=" << i << ", char=" << s_in[i] << ", isF=" << isFString << std::endl;
	while (i < size)
	{
		char c = s_in[i];
		bool bTrigger = false;
		if (isFString)
		{
			if (c == '{')
			{
				if (i + 1 < size && s_in[i + 1] == '{')
				{
					str0 += '{';
					i += 2;
					continue;
				}
				bTrigger = true;
				i++;//skip {
			}
			else if (c == '$' && i + 1 < size && s_in[i + 1] == '{')
			{
				bTrigger = true;
				i += 2;//skip ${
			}
		}
		else
		{
			if (c == '$' && i + 1 < size && s_in[i + 1] == '{')
			{
				bTrigger = true;
				i += 2;//skip ${
			}
		}

		if (bTrigger)
		{
			//find end of } handling nesting
			int j = i;
			int braceCnt = 1;
			bool inQuote = false;
			char quoteChar = 0;
			while (j < size && braceCnt > 0)
			{
				char cc = s_in[j];
				if (inQuote)
				{
					if (cc == '\\')
					{
						j++;
					}
					else if (cc == quoteChar)
					{
						inQuote = false;
					}
				}
				else
				{
					if (cc == '"' || cc == '\'')
					{
						inQuote = true;
						quoteChar = cc;
					}
					else if (cc == '{')
					{
						braceCnt++;
					}
					else if (cc == '}')
					{
						braceCnt--;
					}
				}
				if (braceCnt > 0)
				{
					j++;
				}
			}
			if (braceCnt == 0) // Found matching } at j
			{
				int len = j - i;
				if (len > 0)
				{
					std::string code(s_in + i, len);
					X::Value retVal;
					bool bEvalOK = false;
					
					AST::Module* pTempModule = new AST::Module();
					
					Scope* pMyScope = GetScope();
					if(!pMyScope)
					{
						Expression* p = m_parent;
						while(p)
						{
							pMyScope = p->GetScope();
							if(pMyScope) break;
							p = p->GetParent();
						}
					}
					
					if (pMyScope)
					{
						pTempModule->SetScope(pMyScope);
						X::Parser parser;
						if (parser.Init())
						{
							parser.Compile(pTempModule, (char*)code.c_str(), (int)code.size());
							auto& body = pTempModule->GetBody();
							if (body.size() > 0)
							{
								auto* exp = body[0];
								//bind scope for variables
								//Use pMyScope (the execution context).
								exp->SetScope(pMyScope);
								
								//We need to layout the expression to resolve variables to indices
								exp->ScopeLayout();
								
								ExecAction action;
								if (exp->Exec(rt, action, pContext, retVal))
								{
									bEvalOK = true;
								}
							}
						}
					}
					delete pTempModule;

					if (bEvalOK)
					{
						if (UseBindMode)
						{
							str0 += "?";
							bind_data_list.push_back(retVal);
						}
						else
						{
							str0 += retVal.ToString();
						}
					}
				}
				i = j + 1;//skip }
				continue;
			}
			else
			{
				//Not closed...
			}
		}

		//Normal char processing (escape handling)
		
		if ((!bMeetSlash) && (c == '\\'))
		{
			bMeetSlash = true;
			i++;
			continue;
		}
		else
		{//https://www.w3schools.com/python/gloss_python_escape_characters.asp
		 //just use python's string escape style
			if (bMeetSlash)
			{
				switch (c)
				{
				case '\'':
					break;
				case '\\':
					//meet again, check if it is octal or hex
					if (IsOctal)
					{//come here,must be cases: \o,\oo,\ooo
						//end Octal value
						IsOctal = false;
						c = (char)ecsVal;
						str0 += c;
						digitCount = 0;
						ecsVal = 0;
						//keep bMeetSlash as true for next one
						i++;
						continue;
					}
					else if (IsHex)
					{
						c = (char)ecsVal;
						str0 += c;
						IsHex = false;
						digitCount = 0;
						ecsVal = 0;
						//keep bMeetSlash as true for next one
						i++;
						continue;
					}
					//then will output '\'
					break;
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 't':
					c = '\t';
					break;
				case 'b':
					c = '\b';
					break;
				case 'f':
					c = '\f';
					break;
				case 'x':
				case 'X':
					IsHex = true;
					i++;
					continue;
				default:
					if (c >= '0' && c <= '7')
					{
						if (!IsHex)
						{
							IsOctal = true;
							ecsVal = ecsVal*8+(c - '0');
							digitCount++;
							if (digitCount < 3)
							{
								i++;
								continue;
							}
						}
					}
					if (IsOctal)
					{//come here,must be cases: \o,\oo,\ooo
						//end Octal value
						IsOctal = false;
						c = (char)ecsVal;
						str0 += c;
						bMeetSlash = false;
						digitCount = 0;
						ecsVal = 0;
						i++;
						continue;
					}
					//for hex
					if (IsHex)
					{
						if ((c >= '0' && c <= '9')
							|| (c >= 'a' && c <= 'f')
							|| (c >= 'A' && c <= 'F'))
						{
							int v0 = 0;
							if (c >= '0' && c <= '9') v0 = c - '0';
							else if (c >= 'a' && c <= 'f') v0 = 10 + c - 'a';
							else v0 = 10 + c - 'A';
							ecsVal = ecsVal*16+v0;
							digitCount++;
							if (digitCount == 2)
							{
								c = (char)ecsVal;
								str0 += c;
								bMeetSlash = false;
								IsHex = false;
								digitCount = 0;
								ecsVal = 0;
							}
							i++;
							continue;
						}
						else if (digitCount > 0)
						{//end hex, for case with one hex
							c = (char)ecsVal;
							str0 += c;
							bMeetSlash = false;
							IsHex = false;
							digitCount = 0;
							ecsVal = 0;
							i++;
							continue;
						}
						else
						{
							//error,just output this c
							bMeetSlash = false;
							IsHex = false;
							digitCount = 0;
							ecsVal = 0;
						}
					}
					break;
				}
			}
			str0 += c;
			bMeetSlash = false;
		}
		i++;
	}
	return true;
}

bool ImaginaryNumber::Exec(XlangRuntime* rt, ExecAction& action,
	XObj* pContext, Value& v, LValue* lValue)
{
	X::Data::Complex* pComplexObj = new X::Data::Complex(0, m_val);
	v = X::Value(pComplexObj);
	return true;
}
bool Str::Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue)
{
	if (m_haveFormat)
	{
		bool bOK = RunWithFormat(rt, pContext, v);
		if(!bOK)
		{
			LOG << "Str::Exec calling RunWithFormat returned FALSE" << LINE_END;
		}
		return bOK;
	}
	else if (m_isCharSequence && m_size == 1)
	{
		long long lv = m_s[0];
		v = Value(lv);
		return true;
	}
	else
	{
		v = Value(m_s, m_size);
		return true;
	}
}

bool Str::RunWithFormat(XlangRuntime* rt, XObj* pContext, Value& v)
{
	std::string stOut;
	std::vector<Value> val_list;
	bool bOK = RunStringExpWithFormat(rt, pContext, m_s, m_size, stOut, m_isFString, false,
		val_list);

	if (bOK)
	{
		Data::Str* pStrObj = new Data::Str(stOut);
		v = Value(pStrObj);
	}
	return bOK;
}
FORCE_INLINE bool List::Exec(XlangRuntime* rt, ExecAction& action,
	XObj* pContext, Value& v, LValue* lValue)
{
	X::Data::List* pOutList = new X::Data::List();
	for (auto& item : list)
	{
		Value v0;
		ExecAction action0;
		if (ExpExec(item,rt, action0, pContext, v0))
		{
			pOutList->Add(rt, v0);
		}
	}
	v = X::Value(pOutList);
	return true;
}
}
}