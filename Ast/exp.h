#pragma once
#include "def.h"
#include <string>
#include <vector>
#include "value.h"
#include "lvalue.h"
#include "glob.h"
#include "token.h"
#include "XLangStream.h"

namespace X 
{
class Runtime;
namespace AST
{
enum class ObType
{
	Base,
	Assign,
	BinaryOp,
	UnaryOp,
	PipeOp,
	In,
	Range,
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
	BuiltinFunc,
	Module,
	Block,
	Class,
	From,
	ColonOp,
	CommaOp,
	SemicolonOp,
	As,
	For,
	While,
	If,
	ExternDecl,
	Thru,
	Import
};
class Func;
class Scope;
class Var;

class Expression
{
protected:
	Expression* m_parent = nil;
	Scope* m_scope = nil;//set by compiling
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
	static Expression* CreateByType(ObType t);
	template<typename T>
	T* BuildFromStream(X::XLangStream& stream)
	{
		Expression* pRetExp = nullptr;
		ExpId  Id = 0;
		stream >> Id;
		if (Id)
		{
			ObType ty;
			stream >> ty;
			pRetExp = CreateByType(ty);
			if (pRetExp)
			{
				pRetExp->FromBytes(stream);
				stream.ScopeSpace().Add(Id, pRetExp);
			}
		}
		return pRetExp?dynamic_cast<T*>(pRetExp):nullptr;
	}
	bool SaveToStream(Runtime* rt, XObj* pContext,Expression* pExp, X::XLangStream& stream)
	{
		if (pExp)
		{
			stream << pExp->ID();
			pExp->ToBytes(rt,pContext,stream);
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
	inline void SetHint(int startLine, int endLine, int charPos,
		int charStart,int charEnd)
	{
		m_lineStart = startLine;
		m_lineEnd = endLine;
		m_charPos = charPos;
		m_charStart = charStart;
		m_charEnd = charEnd;
	}
	inline void ReCalcHint(Expression* pAnotherExp)
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
	inline bool IsLeftValue() { return m_isLeftValue; }
	inline int GetStartLine() { return m_lineStart+1; }
	inline int GetEndLine() { return m_lineEnd+1; }
	inline int GetCharPos() { return m_charPos; }
	inline int GetCharStart() { return m_charStart; }
	inline int GetCharEnd() { return m_charEnd; }
	inline void SetIsLeftValue(bool b)
	{
		m_isLeftValue = b;
	}
	virtual ~Expression(){}
	inline virtual Scope* GetScope()
	{
		if (m_scope == nil)
		{
			m_scope = FindScope();//FindScope will AddRef
		}
		return m_scope;
	}
	Scope* FindScope();
	void SetParent(Expression* p)
	{
		m_parent = p;
	}
	Expression* GetParent()
	{
		return m_parent;
	}
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<Scope*>& callables)
	{
		return false;
	}
	virtual void Set(Runtime* rt,XObj* pContext, Value& v){}
	virtual bool Run(Runtime* rt,XObj* pContext,Value& v,LValue* lValue=nullptr)
	{
		return false;
	}
	virtual bool EatMe(Expression* other)
	{
		return false;
	}
	virtual void ScopeLayout() {}
	virtual int GetLeftMostCharPos() { return m_charPos; }
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream);
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
	ObType m_type = ObType::Base;
};
class Str :
	virtual public Expression
{
	bool m_haveFormat = false;
	char* m_s = nil;
	bool m_needRelease = false;//m_s is created by this Str,then = true
	int m_size = 0;
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
	~Str()
	{
		if (m_needRelease)
		{
			delete m_s;
		}
	}
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
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
	bool RunWithFormat(Runtime* rt, XObj* pContext, Value& v, LValue* lValue);
	virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override
	{
		if (m_haveFormat)
		{
			return RunWithFormat(rt, pContext, v, lValue);
		}
		else
		{
			v = Value(m_s, m_size);
			return true;
		}
	}
};
class XConst :
	virtual public Expression
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
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
	virtual bool Run(Runtime* rt, XObj* pContext, Value& v, LValue* lValue = nullptr) override
	{
		if (m_tokenIndex == TokenIndex::Token_None)
		{
			v.SetType(ValueType::None);
		}
		return true;
	}
};
class Number :
	virtual public Expression
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
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
	inline long long GetVal() { return m_val; }
	inline int GetDigiNum() { return m_digiNum; }
	virtual bool Run(Runtime* rt,XObj* pContext, Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_val);
		if (m_isBool)
		{
			v0.AsBool();
		}
		else
		{
			v0.SetF(m_digiNum);
		}
		v = v0;
		return true;
	}
};
class Double :
	virtual public Expression
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
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
};
class List :
	virtual public Expression
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
	virtual bool ToBytes(Runtime* rt,XObj* pContext,X::XLangStream& stream)
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
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
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
class Param :
	virtual public Expression
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
	inline Expression* GetName() { return Name; }
	inline Expression* GetType() { return Type; }
	bool Parse(std::string& strVarName,
		std::string& strVarType,
		Value& defaultValue);
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
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