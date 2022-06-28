#include "exp.h"
#include "builtin.h"
#include <iostream>
#include "object.h"
#include "runtime.h"
#include "op.h"
#include "var.h"
#include "block.h"
#include "func.h"
#include "str.h"
#include "utility.h"

namespace X 
{
namespace AST 
{
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
bool Str::RunWithFormat(Runtime* rt, void* pContext, Value& v, LValue* lValue)
{
	std::string str0;
	bool bMeetSlash = false;
	bool IsOctal = false;
	int ecsVal = 0;
	int digitCount = 0;
	bool IsHex = false;
	int i = 0;
	while(i<m_size)
	{
		char c = m_s[i++];
		if (c == '$')
		{
			if(i < m_size)
			{
				if (m_s[i] == '{')
				{//enter case $${var}
					int j = i;
					bool bMeetEnd = false;
					while (j < m_size)
					{
						if (m_s[j] == '}')
						{
							bMeetEnd = true;
							break;
						}
						j++;
					}
					if (bMeetEnd)
					{
						bool bGotVal = false;
						std::string strPart;
						Scope* pMyScope = GetScope();
						if (pMyScope)
						{
							std::string varName(m_s + i + 1, j - i - 1);
							std::vector<std::string> listVars 
								= split(varName, ',');
							for (auto it : listVars)
							{
								int idx = pMyScope->AddOrGet(it, true);
								if (idx >= 0)
								{
									Value v0;
									if (pMyScope->Get(rt, pContext, idx, v0))
									{
										strPart += v0.ToString();
										bGotVal = true;
									}
								}
								else if (it == "&COMMA" || it == "&comma"
									||it =="\\")//for cause {\,,var}
								{
									strPart += ",";
								}
								else if (it == "\\n")
								{
									strPart += '\n';
								}
								else if (it == "\\t")
								{
									strPart += '\t';
								}
								else if (it == "\\r")
								{
									strPart += '\r';
								}
								else
								{//if not find, just out as a string
								//usage ${my name is,Name},but can't use ,
									strPart += it;
								}
							}
						}
						//at least find one var, then bGotVal is true
						if (bGotVal)
						{
							str0 += strPart;
							i = j + 1;
							continue;
						}
						//if not, treat as char to output
					}
				}
			}
		}
		if ((!bMeetSlash) && (c == '\\'))
		{
			bMeetSlash = true;
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
	}
	Data::Str* pStrObj = new Data::Str(str0);
	v = Value(pStrObj);
	return true;
}
}
}