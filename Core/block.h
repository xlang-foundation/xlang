#pragma once
#include "exp.h"
#include "op.h"

namespace X
{
namespace AST
{
struct Indent
{
	int charPos = 0;
	int tab_cnt =0;
	int space_cnt =0;
	bool operator>=(const Indent& other)
	{
		return (charPos>=other.charPos)
			&& (tab_cnt >= other.tab_cnt) 
			&& (space_cnt >= other.space_cnt);
	}
	bool operator==(const Indent& other)
	{
		return (charPos == other.charPos)
			&& (tab_cnt == other.tab_cnt)
			&& (space_cnt == other.space_cnt);
	}
	bool operator<(const Indent& other)
	{
		return (charPos <= other.charPos) &&
			((tab_cnt <= other.tab_cnt && space_cnt < other.space_cnt)
				|| (tab_cnt < other.tab_cnt&& space_cnt <= other.space_cnt));
	}
};
class Block:
	public UnaryOp
{
	bool NoIndentCheck = false;//just for lambda block
	Indent IndentCount = { 0,-1,-1 };
	Indent ChildIndentCount = { 0,-1,-1 };
	std::vector<Expression*> Body;
public:
	Block()
	{
	}
	Block(short op) :
		UnaryOp(op)
	{
	}

	inline bool IsNoIndentCheck()
	{
		return NoIndentCheck;
	}
	inline void SetNoIndentCheck(bool b)
	{
		NoIndentCheck = b;
	}
	~Block()
	{
		for (auto b : Body)
		{
			delete b;
		}
	}
	virtual void Add(Expression* item);
	virtual Func* FindFuncByName(Var* name);
	inline Indent GetIndentCount() { return IndentCount; }
	inline Indent GetChildIndentCount() { return ChildIndentCount; }
	inline void SetIndentCount(Indent cnt) { IndentCount = cnt; }
	inline void SetChildIndentCount(Indent cnt) { ChildIndentCount = cnt; }
	virtual bool Run(Runtime* rt,void* pContext, Value& v, LValue* lValue = nullptr);
};
class For :
	public Block
{
public:
	For(short op):
		Block(op)
	{
	}
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override;
};
class While :
	public Block
{
public:
	While(short op) :
		Block(op)
	{
	}

	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override;
};

class If :
	public Block
{
	If* m_next = nil;//elif  or else
public:
	If(short op,bool needParam =true) :
		Block(op)
	{
		NeedParam = needParam;
	}
	~If()
	{
		if (m_next) delete m_next;
	}
	virtual bool EatMe(Expression* other) override;
	virtual bool Run(Runtime* rt,void* pContext, Value& v,LValue* lValue=nullptr) override;
};
}
}