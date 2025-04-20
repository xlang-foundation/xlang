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

#pragma once
//#include "def.h"
#include <string>
#include <vector>
#include "value.h"
#include "lvalue.h"
#include "glob.h"
#include "token.h"
#include "XLangStream.h"

namespace X 
{
class XlangRuntime;
namespace AST
{
enum class ObType
{
	Base,
	InlineComment,
	Assign,
	BinaryOp,
	UnaryOp,
	PipeOp,
	In,
	Var,
	Str,
	Const,
	Number,
	Double,
	Param,
	List,
	Pair,
	Dot,
	Decor,
	Func,
	JitBlock,
	BuiltinFunc,
	Module,
	Block,
	Class,
	ReturnType,
#if ADD_SQL
	Select,
#endif
	From,
	ColonOp,
	CommaOp,
	SemicolonOp,
	FeedOp,
	ActionOp,//for break,continue and pass
	As,
	For,
	While,
	If,
	ExternDecl,
	AwaitOp,
	Thru,
	Deferred,
	Import,
	NamespaceVar,
	RefOp,
};
enum class ExecActionType
{
	None,
	Break,
	Continue,
	Return,
};

struct ExecAction
{
	ExecActionType type = ExecActionType::None;
};
class Func;
class Scope;
class Var;


//About **scope**,each Expression has a scope which is a refrerence to a real scope
//only Func,Class,Module etc has a real scope, will be its member m_pMyScope
//for real scope, we will use term MyScope, and put a variable m_pMyScope in Expression
//we don't use virtual function to get scope which will bring optinization problem
class Expression
{
protected:
	Scope* m_pMyScope = nullptr;

	int m_tokenIndex = -1;
	Expression* m_parent = nil;
	//m_scope points to the scope for this expression which has a name need to translate into index
	Scope* m_scope = nil;
	bool m_isLeftValue = false;
	//hint
	int m_lineStart=-1;
	int m_lineEnd=-1;
	int m_charPos=-1;
	int m_charStart = -1;//offset in entire code
	int m_charEnd = -1;
public:
	Expression()
	{
	}
	FORCE_INLINE void SetTokenIndex(int idx)
	{
		m_tokenIndex = idx;
	}
	FORCE_INLINE int GetTokenIndex() { return m_tokenIndex; }
	static Expression* CreateByType(ObType t);
	template<typename T>
	T* BuildFromStream(X::XLangStream& stream)
	{
		Expression* pRetExp = nullptr;
		ExpId  Id = 0;
		stream >> Id;
		if (Id)
		{
			bool bIsRef = false;
			stream >> bIsRef;
			if (bIsRef)
			{
				pRetExp = (Expression*)stream.ScopeSpace().Query(Id);
			}
			else
			{
				ObType ty;
				stream >> ty;
				pRetExp = CreateByType(ty);
				if (pRetExp)
				{
					//add it into map first, then child item can find parent
					stream.ScopeSpace().Add(Id, pRetExp);
					pRetExp->FromBytes(stream);
				}
			}
		}
		return pRetExp?dynamic_cast<T*>(pRetExp):nullptr;
	}
	bool SaveToStream(XlangRuntime* rt, XObj* pContext,Expression* pExp, X::XLangStream& stream)
	{
		if (pExp)
		{
			auto id = pExp->ID();
			stream << id;
			//bHaveIt acts as a flag to indicate if this expression is referenced 
			//or will save itself after this flag
			bool bHaveIt = (stream.ScopeSpace().Query(id) != nullptr);
			stream << bHaveIt;
			if (!bHaveIt)
			{
				stream.ScopeSpace().Add(id, pExp);
				pExp->ToBytes(rt, pContext, stream);
			}
		}
		else
		{
			stream << nullptr;
		}
		return true;
	}
	std::string GetCode();
	//use address as ID, just used Serialization
	ExpId ID() { return (ExpId)this; }
	FORCE_INLINE void SetHint(int startLine, int endLine, int charPos,
		int charStart,int charEnd)
	{
		m_lineStart = startLine;
		m_lineEnd = endLine;
		m_charPos = charPos;
		m_charStart = charStart;
		m_charEnd = charEnd;
	}
	FORCE_INLINE void ReCalcHint(Expression* pAnotherExp)
	{
		auto min_val = [](int x, int y) {
			return x >= 0 ? MIN_VAL(x, y) : y;
		};
		auto max_val = [](int x, int y) {
			return x >= 0 ? MAX_VAL(x, y) : y;
		};

		m_lineStart = min_val(m_lineStart, pAnotherExp->m_lineStart);
		m_lineEnd = max_val(m_lineEnd, pAnotherExp->m_lineEnd);
		m_charPos = min_val(m_charPos, pAnotherExp->m_charPos);
		m_charStart = min_val(m_charStart, pAnotherExp->m_charStart);
		m_charEnd = max_val(m_charEnd, pAnotherExp->m_charEnd);
	}
	FORCE_INLINE bool IsLeftValue() { return m_isLeftValue; }
	FORCE_INLINE int GetStartLine() { return m_lineStart+1; }
	FORCE_INLINE int GetEndLine() { return m_lineEnd+1; }
	FORCE_INLINE int GetCharPos() { return m_charPos; }
	FORCE_INLINE int GetCharStart() { return m_charStart; }
	FORCE_INLINE int GetCharEnd() { return m_charEnd; }
	FORCE_INLINE virtual void SetIsLeftValue(bool b)
	{
		m_isLeftValue = b;
	}
	virtual ~Expression(){}
	FORCE_INLINE Scope* GetMyScope()
	{
		return m_pMyScope;
	}
	FORCE_INLINE virtual Scope* GetScope()
	{
		if (m_scope == nil)
		{
			m_scope = FindScope();//FindScope will AddRef
		}
		return m_scope;
	}
	Module* FindModule()
	{
		Module* pModule = nullptr;
		Expression* pa = this;
		while (pa != nullptr)
		{
			if (pa->m_type == ObType::Module)
			{
				pModule = (Module*)pa;
				break;
			}
			pa = pa->GetParent();
		}
		return pModule;
	}
	//Find MyScope from ancestor wich has RealScope
	Scope* FindScope()
	{
		Scope* pMyScope = nullptr;
		Expression* pa = m_parent;
		while (pa != nullptr)
		{
			pMyScope = pa->GetMyScope();
			if (pMyScope)
			{
				break;
			}
			pa = pa->GetParent();
		}
		return pMyScope;
	}
	//Set Expresion's scope which is a reference to a real scope
	virtual void SetScope(Scope* p)
	{
		m_scope = p;
	}
	void SetParent(Expression* p)
	{
		m_parent = p;
	}
	FORCE_INLINE Expression* GetParent()
	{
		return m_parent;
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables)
	{
		return false;
	}
	virtual bool Set(XlangRuntime* rt, XObj* pContext, Value& v) { return true; }
	virtual bool SetArry(XlangRuntime* rt, XObj* pContext, std::vector<Value>& ary) { return true; }
	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext,Value& v,LValue* lValue=nullptr)
	{
		return false;
	}
	virtual bool EatMe(Expression* other)
	{
		return false;
	}
	virtual void ScopeLayout() {}
	virtual int GetLeftMostCharPos() { return m_charPos; }
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream);
	virtual bool FromBytes(X::XLangStream& stream);
	virtual void ConvertIDToRealAddress(std::unordered_map<ExpId,void*>& addressMap)
	{
		if (m_parent)
		{
			auto it = addressMap.find((ExpId)m_parent);
			if (it != addressMap.end())
			{
				m_parent = (Expression*)it->second;
			}
		}
		if (m_scope)
		{
			auto it = addressMap.find((ExpId)m_scope);
			if (it != addressMap.end())
			{
				m_scope = (Scope*)it->second;
			}
		}
	}
	bool RunStringExpWithFormat(XlangRuntime* rt, XObj* pContext,
		const char* s_in,int size,std::string& outStr,bool UseBindMode,
		std::vector<X::Value>& bind_data_list);
	ObType m_type = ObType::Base;
};
//only keep for AST Query
class InlineComment:
	public Expression
{
	char* m_s = nil;
	int m_size = 0;
public:
	InlineComment() :Expression()
	{
	}
	InlineComment(char* s, int size):
		InlineComment()
	{
		m_type = ObType::InlineComment;
		m_s = s;
		m_size = size;
	}
	std::string GetString() { return std::string(m_s,m_size); }
};
class Str :
	public Expression
{
	bool m_haveFormat = false;
	char* m_s = nil;
	bool m_needRelease = false;//m_s is created by this Str,then = true
	int m_size = 0;
	bool m_isCharSequence = false;//'.....'
public:
	Str():Expression()
	{
		m_type = ObType::Str;
	}
	Str(char* s, int size,bool haveFormat)
	{
		m_type = ObType::Str;
		m_haveFormat = haveFormat;
		m_s = s;
		m_needRelease = false;
		m_size = size;
	}
	FORCE_INLINE bool IsCharSequence() { return m_isCharSequence; }
	FORCE_INLINE int Size() { return m_size; }
	FORCE_INLINE char* GetChars() { return m_s; }
	void SetCharFlag(bool isChars)
	{
		m_isCharSequence = isChars;
	}
	~Str()
	{
		if (m_needRelease)
		{
			delete m_s;
		}
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
	{
		Expression::ToBytes(rt, pContext,stream);
		stream << m_haveFormat;
		stream << m_size;
		stream.append(m_s, m_size);
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> m_haveFormat;
		stream >> m_size;
		if (m_size > 0)
		{
			m_s = new char[m_size];
			m_needRelease = true;
			stream.CopyTo(m_s, m_size);
		}
		else
		{
			m_s = nullptr;
		}
		return true;
	}
	bool RunWithFormat(XlangRuntime* rt, XObj* pContext, Value& v);

	virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (m_haveFormat)
		{
			return RunWithFormat(rt, pContext, v);
		}
		else if (m_isCharSequence && m_size == 1)
		{//for case: 'A', 'a'...., return a long long as its value
		 //todo: check if it can be accept as +='s right value for example:
		// str_x +='A'
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
};

//deal with built-in constants such as None
//But True and False deal by Number not XConst

class XConst :
	public Expression
{
	int m_tokenIndex;
public:
	XConst():Expression()
	{
		m_type = ObType::Const;
	}
	XConst(int t)
	{
		m_tokenIndex = t;
		m_type = ObType::Const;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
	{
		Expression::ToBytes(rt,pContext,stream);
		stream << m_tokenIndex;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> m_tokenIndex;
		return true;
	}
	FORCE_INLINE virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override
	{
		if (m_tokenIndex == TokenIndex::Token_None)
		{
			v.SetType(ValueType::None);
		}
		return true;
	}
};
class Number :
	public Expression
{
	long long m_val;
	int m_digiNum = 0;
	bool m_isBool = false;
public:
	Number():Expression()
	{
		m_type = ObType::Number;
	}
	Number(long long val, int num=0)
	{
		m_val = val;
		m_digiNum = num;
		m_type = ObType::Number;
	}
	Number(bool val)
	{
		m_val = val?1:0;
		m_type = ObType::Number;
		m_isBool = true;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
	{
		Expression::ToBytes(rt,pContext,stream);
		stream << m_val<< m_digiNum<< m_isBool;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> m_val >> m_digiNum >> m_isBool;
		return true;
	}
	FORCE_INLINE long long GetVal() { return m_val; }
	FORCE_INLINE int GetDigiNum() { return m_digiNum; }
	FORCE_INLINE virtual bool Exec(XlangRuntime* rt,ExecAction& action,XObj* pContext, Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_val);
		if (m_isBool)
		{
			v0.AsBool();
		}
		else
		{
			v0.SetDigitNum(m_digiNum);
		}
		v = v0;
		return true;
	}
	FORCE_INLINE X::Value GetValue()
	{
		Value v0(m_val);
		if (m_isBool)
		{
			v0.AsBool();
		}
		else
		{
			v0.SetDigitNum(m_digiNum);
		}
		return v0;
	}

};
class Double :
	public Expression
{
	double m_val=0;
public:
	Double()
	{
		m_type = ObType::Double;
	}
	Double(double val)
	{
		m_val = val;
		m_type = ObType::Double;
	}
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
	{
		Expression::ToBytes(rt,pContext,stream);
		stream << m_val;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> m_val;
		return true;
	}
	FORCE_INLINE double GetVal() { return m_val; }
};
//Only the imaginary  part for Complex
class ImaginaryNumber :
	public Expression
{
	double m_val = 0;
public:
	ImaginaryNumber()
	{
		m_type = ObType::Double;
	}
	ImaginaryNumber(double val)
	{
		m_val = val;
		m_type = ObType::Double;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream)
	{
		Expression::ToBytes(rt, pContext, stream);
		stream << m_val;
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		stream >> m_val;
		return true;
	}
	virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr) override final;
};

class List :
	public Expression
{
	std::vector<Expression*> list;
public:
	List()
	{
		m_type = ObType::List;
	}
	void ClearList() { list.clear();}//before this call,
	//copy all list into another List
	~List()
	{
		for (auto e : list)
		{
			delete e;
		}
	}
	virtual bool Exec(XlangRuntime* rt, ExecAction& action,
		XObj* pContext, Value& v, LValue* lValue = nullptr) override;
	virtual bool ToBytes(XlangRuntime* rt,XObj* pContext,X::XLangStream& stream)
	{
		Expression::ToBytes(rt,pContext,stream);
		stream << (int)list.size();
		for (auto* exp : list)
		{
			SaveToStream(rt, pContext,exp, stream);
		}
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream)
	{
		Expression::FromBytes(stream);
		int size = 0;
		stream >> size;
		for (int i = 0; i < size; i++)
		{
			auto* exp = BuildFromStream<Expression>(stream);
			list.push_back(exp);
		}
		return true;
	}
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
	{
		bool bHave = false;
		for (auto it : list)
		{
			bHave |= it->CalcCallables(rt, pContext, callables);
		}
		return bHave;
	}
	virtual int GetLeftMostCharPos() override
	{
		if (list.size() > 0)
		{
			return list[0]->GetLeftMostCharPos();
		}
		else
		{
			return 9999;
		}
	}
	virtual void ScopeLayout() override
	{
		for (auto i : list)
		{
			i->ScopeLayout();
		}
	}
	std::vector<Expression*>& GetList()
	{
		return list;
	}
	List(Expression* item):List()
	{
		ReCalcHint(item);
		list.push_back(item);
		if (item)
		{
			item->SetParent(this);
		}
	}
	List& operator+=(const List& rhs)
	{
		for (auto i : rhs.list)
		{
			ReCalcHint(i);
			list.push_back(i);
			i->SetParent(this);
		}
		return *this;
	}
	List& operator+=(Expression* item)
	{
		ReCalcHint(item);
		list.push_back(item);
		if (item)
		{
			item->SetParent(this);
		}
		return *this;
	}
};
//use Param for this syntax
// name:type for example in function define 
// def func1(x:int)
//but also can be some other meaning bases on the context
class Param :
	public Expression
{
	Expression* Name = nil;
	Expression* Type = nil;
public:
	Param() :Expression()
	{
		m_type = ObType::Param;
	}
	Param(Expression* name, Expression* type)
	{
		Name = name;
		Type = type;
		if (Name)
		{
			Name->SetParent(this);
		}
		if (Type)
		{
			Type->SetParent(this);
		}
		m_type = ObType::Param;
	}
	~Param()
	{
		if (Name) delete Name;
		if (Type) delete Type;
	}
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		Expression::ToBytes(rt, pContext, stream);
		SaveToStream(rt, pContext, Name, stream);
		SaveToStream(rt, pContext, Type, stream);
		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Expression::FromBytes(stream);
		Name = BuildFromStream<Expression>(stream);
		Type = BuildFromStream<Expression>(stream);
		return true;
	}
	FORCE_INLINE Expression* GetName() { return Name; }
	FORCE_INLINE Expression* GetType() { return Type; }
	bool Parse(std::string& strVarName,
		std::string& strVarType,
		Value& defaultValue);
	virtual bool Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext, Value& v, LValue* lValue = nullptr);
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override
	{
		bool bHave = Name ? Name->CalcCallables(rt, pContext, callables) : false;
		bHave |= Type ? Type->CalcCallables(rt, pContext, callables) : false;
		return bHave;
	}
	virtual void ScopeLayout() override
	{
		if (Name) Name->ScopeLayout();
		if (m_parent->m_type != ObType::Class)
		{
			if (Type) Type->ScopeLayout();
		}
	}
};
}
}