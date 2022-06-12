#pragma once

#include "pycore.h"
#include "def.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include "value.h"
#include "stackframe.h"
#include "glob.h"

namespace X {namespace AST{
class List;
typedef bool (*U_FUNC) (std::vector<Value>& params, Value& retValue);
enum class ObType
{
	Base,
	Assign,
	BinaryOp,
	UnaryOp,
	Range,
	Var,
	Number,
	Double,
	Param,
	List,
	Pair,
	Dot,
	Func,
	Class
};

class Scope;
class Var;
class Func;
class Expression
{
protected:
	Expression* m_parent = nil;
	Scope* m_scope = nil;//set by compiling
	bool m_isLeftValue = false;
	//hint
	int m_lineStart=0;
	int m_lineEnd=0;
	int m_charPos=0;
public:
	Expression()
	{
	}
	inline void SetHint(int startLine, int endLine, int charPos)
	{
		m_lineStart = startLine;
		m_lineEnd = endLine;
		m_charPos = charPos;
	}
	inline int GetStartLine() { return m_lineStart; }
	inline int GetEndLine() { return m_lineEnd; }
	inline int GetCharPos() { return m_charPos; }
	inline void SetIsLeftValue(bool b)
	{
		m_isLeftValue = b;
	}
	virtual ~Expression(){}
	inline Scope* GetScope()
	{
		if (m_scope == nil)
		{
			m_scope = FindScope();//FindScope will AddRef
		}
		return m_scope;
	}
	Scope* FindScope();
	Func* FindFuncByName(Var* name);
	void SetParent(Expression* p)
	{
		m_parent = p;
	}
	Expression* GetParent()
	{
		return m_parent;
	}
	virtual void Set(Value& v)
	{

	}
	virtual bool Run(Value& v,LValue* lValue=nullptr)
	{
		return false;
	}
	virtual bool EatMe(Expression* other)
	{
		return false;
	}
	virtual void ScopeLayout() {}

	ObType m_type = ObType::Base;
};
class Operator :
	public Expression
{
protected:
	short Op;//index of _kws
public:
	Operator()
	{
		Op = 0;
	}
	Operator(short op)
	{
		Op = op;
	}

	inline short getOp(){return Op;}
	virtual void SetL(Expression* l) {}
	virtual void SetR(Expression* r) {}
	virtual void OpWithOperands(std::stack<AST::Expression*>& operands) {}
};

class BinaryOp :
	public Operator
{
protected:
	Expression* L=nil;
	Expression* R = nil;
public:
	BinaryOp(short op):
		Operator(op)
	{
		m_type = ObType::BinaryOp;
	}
	~BinaryOp()
	{
		if (L) delete L;
		if (R) delete R;
	}
	virtual void OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		auto operandR = operands.top();
		operands.pop();
		SetR(operandR);
		auto operandL = operands.top();
		operands.pop();
		SetL(operandL);
		operands.push(this);
	}
	virtual void ScopeLayout() override
	{
		if (L) L->ScopeLayout();
		if (R) R->ScopeLayout();
	}
	virtual void SetL(Expression* l) override
	{
		L = l;
		if (L) 
		{
			L->SetParent(this);
		}
	}
	virtual void SetR(Expression* r) override
	{
		R = r;
		if (R)
		{
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }
	Expression* GetL() { return L; }

	virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		if (!L->Run(v_l))
		{
			return false;
		}
		Value v_r;
		if (!R->Run(v_r))
		{
			return false;
		}
		auto func = G::I().OpAct(Op).binaryop;
		return func ? func(this, v_l, v_r, v) : false;
	}
};
class Assign :
	public BinaryOp
{
public:
	Assign(short op) :
		BinaryOp(op)
	{
		m_type = ObType::Assign;
	}
	virtual void SetL(Expression* l) override
	{
		BinaryOp::SetL(l);
		if (L)
		{
			L->SetIsLeftValue(true);
		}
	}
	virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		if (!L || !R)
		{
			return false;
		}
		Value v_l;
		LValue lValue_L = nullptr;
		L->Run(v_l, &lValue_L);
		Value v_r;
		if (R->Run(v_r))
		{
			if (lValue_L)
			{
				*lValue_L = v_r;
			}
			else
			{
				L->Set(v_r);
			}
		}
		return true;
	}
};
class ColonOP :
	public Operator
{
public:
	ColonOP(short op) :
		Operator(op)
	{
	}
	virtual void OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class CommaOp :
	public Operator
{
public:
	CommaOp(short op) :
		Operator(op)
	{
	}
	virtual void OpWithOperands(
		std::stack<AST::Expression*>& operands);
};
class PairOp :
	public BinaryOp
{
	short m_preceding_token = 0;
	bool GetParamList(Expression* e,
		std::vector<Value>& params,
		std::unordered_map<std::string,Value>& kwParams);
public:
	PairOp(short opIndex, short preceding_token) :
		BinaryOp(opIndex)
	{
		m_preceding_token = preceding_token;
		m_type = ObType::Pair;
	}
	short GetPrecedingToken()
	{
		return m_preceding_token;
	}
	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
};
class DotOp :
	public BinaryOp
{
	int m_dotNum = 1;
public:
	DotOp(short opIndex,int dotNum) :
		BinaryOp(opIndex)
	{
		m_type = ObType::Dot;
		m_dotNum = dotNum;
	}
	virtual bool Run(Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
};
class UnaryOp :
	public Operator
{
protected:
	Expression* R = nil;
	bool NeedParam = true;//like else(), it is false
public:
	UnaryOp()
	{
	}
	UnaryOp(short op):
		Operator(op)
	{
		m_type = ObType::UnaryOp;
	}
	~UnaryOp()
	{
		if (R) delete R;
	}
	virtual void ScopeLayout() override
	{
		if (R) R->ScopeLayout();
	}
	virtual void OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		if (NeedParam)
		{
			auto operandR = operands.top();
			operands.pop();
			SetR(operandR);
		}
		operands.push(this);
	}
	virtual void SetR(Expression* r) override
	{
		R = r;
		if (R)
		{
			R->SetParent(this);
		}
	}
	Expression* GetR() { return R; }

	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
};
class Range :
	public UnaryOp
{
	bool m_evaluated = false;
	long long m_start=0;
	long long m_stop =0;
	long long m_step = 1;

	bool Eval();
public:
	Range(short op) :
		UnaryOp(op)
	{
		m_type = ObType::Range;
	}

	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
};
class Str :
	public Expression
{
	char* m_s = nil;
	int m_size = 0;
public:
	Str(char* s, int size)
	{
		m_s = s;
		m_size = size;
	}
	virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_s,m_size);
		v = v0;
		return true;
	}
};
class Number :
	public Expression
{
	long long m_val;
	int m_digiNum = 0;
public:
	Number(long long val, int num=0)
	{
		m_val = val;
		m_digiNum = num;
		m_type = ObType::Number;
	}
	virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		Value v0(m_val);
		v0.SetF(m_digiNum);
		v = v0;
		return true;
	}
};
class Double :
	public Expression
{
	double m_val;
public:
	Double(double val)
	{
		m_val = val;
		m_type = ObType::Double;
	}
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
			list.push_back(i);
			if (i)
			{
				i->SetParent(this);
			}
		}
		return *this;
	}
	List& operator+=(Expression* item)
	{
		list.push_back(item);
		if (item)
		{
			item->SetParent(this);
		}
		return *this;
	}
};
class Param :
	public Expression
{
	Expression* Name = nil;
	Expression* Type = nil;
public:
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
};
class Func;

struct Indent
{
	int tab_cnt =0;
	int space_cnt =0;
	bool operator>=(const Indent& other)
	{
		return (tab_cnt >= other.tab_cnt) 
			&& (space_cnt >= other.space_cnt);
	}
	bool operator==(const Indent& other)
	{
		return (tab_cnt == other.tab_cnt)
			&& (space_cnt == other.space_cnt);
	}
	bool operator<(const Indent& other)
	{
		return (tab_cnt <= other.tab_cnt && space_cnt < other.space_cnt)
			|| (tab_cnt < other.tab_cnt && space_cnt <= other.space_cnt);
	}
};
class Block:
	public UnaryOp
{
	Indent IndentCount = { -1,-1 };
	Indent ChildIndentCount = { -1,-1 };
	std::vector<Expression*> Body;
public:
	Block()
	{
	}
	Block(short op) :
		UnaryOp(op)
	{
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
	virtual bool Run(Value& v,LValue* lValue=nullptr)
	{
		bool bOk = true;
		for (auto i : Body)
		{
			Value v0;
			bOk = i->Run(v0);
			if (!bOk)
			{
				break;
			}
		}
		return bOk;
	}
};
class InOp :
	public BinaryOp
{
public:
	InOp(short op) :
		BinaryOp(op)
	{
	}
	inline virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		bool bIn = R->Run(v);
		if(bIn)
		{
			L->Set(v);
		}
		return bIn;
	}
	virtual void SetL(Expression* l) override
	{
		BinaryOp::SetL(l);
		if (l)
		{
			l->SetIsLeftValue(true);
		}
	}
};
class For :
	public Block
{
public:
	For(short op):
		Block(op)
	{
	}
	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
};
class While :
	public Block
{
public:
	While(short op) :
		Block(op)
	{
	}

	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
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
	virtual bool Run(Value& v,LValue* lValue=nullptr) override;
};

class Scope:
	public Block
{//variables scope support, for Module and Func/Class
protected:
	std::unordered_map < std::string, int> m_Vars;
	std::stack<StackFrame*> mStackFrames;
public:
	Scope() :
		Block()
	{
	}
	int AddOrGet(std::string& name,bool bGetOnly)
	{//Always append,no remove, so new item's index is size of m_Vars;
		auto it = m_Vars.find(name);
		if (it != m_Vars.end())
		{
			return it->second;
		}
		else if (!bGetOnly)
		{
			int idx = (int)m_Vars.size();
			m_Vars.emplace(std::make_pair(name, idx));
			return idx;
		}
		else
		{
			return -1;
		}
	}
	void PushFrame(StackFrame* frame)
	{
		frame->SetVarCount((int)m_Vars.size());
		mStackFrames.push(frame);
	}
	StackFrame* PopFrame()
	{
		return mStackFrames.empty() ? nil : mStackFrames.top();
	}

	inline bool Set(int idx, Value& v)
	{
		if (!mStackFrames.empty())
		{
			mStackFrames.top()->Set(idx, v);
		}
		return true;
	}
	inline bool SetReturn(Value& v)
	{
		mStackFrames.top()->SetReturn(v);
		return true;
	}
	inline bool Get(int idx, Value& v, LValue* lValue = nullptr)
	{
		if (!mStackFrames.empty())
		{
			mStackFrames.top()->Get(idx, v,lValue);
		}
		return true;
	}
};

class Var :
	public Expression
{
	String Name;
	int Index = -1;//index for this Var,set by compiling
public:
	Var(String& n)
	{
		Name = n;
		m_type = ObType::Var;
	}
	void ScopeLayout(std::vector<AST::Expression*>& candidates)
	{
		std::string strName(Name.s, Name.size);
		for (auto it : candidates)
		{
			Scope* s = dynamic_cast<Scope*>(it);
			if (s)
			{
				int idx = s->AddOrGet(strName,
					!m_isLeftValue);
				if (idx != -1)
				{//use the scope to find this name as its scope
					m_scope = s;
					Index = idx;
					break;
				}
			}
		}
	}

	virtual void ScopeLayout() override
	{
		Scope* pMyScope = GetScope();
		int idx = -1;
		while (pMyScope!= nullptr && idx ==-1)
		{
			std::string strName(Name.s, Name.size);
			idx = pMyScope->AddOrGet(strName,
				!m_isLeftValue);
			if (m_isLeftValue)
			{//Left value will add into local scope
			//don't need to go up
				break;
			}
			if (idx != -1)
			{//use the scope to find this name as its scope
				m_scope = pMyScope;
				break;
			}
			pMyScope = pMyScope->GetScope();
		}
		Index = idx;
	}
	String& GetName() { return Name; }
	inline virtual void Set(Value& v) override
	{
		m_scope->Set(Index,v);
	}
	inline virtual bool Run(Value& v,LValue* lValue=nullptr) override
	{
		if (Index == -1 || m_scope == nullptr)
		{
			return false;
		}
		m_scope->Get(Index, v, lValue);
		return true;
	}
};

class Module :
	public Scope
{
public:
	Module() :
		Scope()
	{
		SetIndentCount({ -1,-1 });//then each line will have 0 indent
	}
	virtual void ScopeLayout() override;
	void AddBuiltins();
};
class Func :
	public Scope
{
protected:
	String m_Name;
	int m_Index = -1;//index for this Var,set by compiling
	int m_positionParamCnt = 0;
	Expression* Name=nil;
	List* Params =nil;
	Expression* RetType = nil;
	void SetName(Expression* n)
	{
		Var* vName = dynamic_cast<Var*>(n);
		if (vName)
		{
			m_Name = vName->GetName();
		}
		Name = n;
		if (Name)
		{
			Name->SetParent(this);
		}
	}
	virtual void ScopeLayout() override;
	virtual void SetParams(List* p)
	{
		Params = p;
		if (Params)
		{
			Params->SetParent(this);
		}
	}
public:
	Func():
		Scope()
	{
		m_type = ObType::Func;
	}
	~Func()
	{
		if (Params) delete Params;
		if (RetType) delete RetType;
	}
	Expression* GetName() { return Name; }
	String& GetNameStr() { return m_Name; }

	virtual void SetR(Expression* r) override
	{
		if (r->m_type == AST::ObType::Pair)
		{//have to be a pair
			AST::PairOp* pair = (AST::PairOp*)r;
			AST::Expression* paramList = pair->GetR();
			if (paramList)
			{
				if (paramList->m_type != AST::ObType::List)
				{
					AST::List* list = new AST::List(paramList);
					SetParams(list);
				}
				else
				{
					SetParams((AST::List*)paramList);
				}
			}
			pair->SetR(nil);//clear R, because it used by SetParams
			AST::Expression* l = pair->GetL();
			if (l)
			{
				SetName(l);
				pair->SetL(nil);
			}
			//content used by func,and clear to nil,
			//not be used anymore, so delete it
			delete pair;
		}
	}

	void SetRetType(Expression* p)
	{
		RetType = p;
		if (RetType)
		{
			RetType->SetParent(this);
		}
	}
	virtual bool Call(std::vector<Value>& params, Value& retValue);
	virtual bool Run(Value& v, LValue* lValue = nullptr) override;
};
class XClass
	:public Func
{
	Func* m_constructor = nil;
	std::vector<XClass*> m_bases;
	std::vector<std::pair<int, Value>> m_tempMemberList;
	XClass* FindBase(std::string& strName);
public:
	XClass():
		Func()
	{
		m_type = ObType::Class;
	}
	inline std::vector<XClass*>& GetBases() { return m_bases; }
	virtual bool Run(Value& v, LValue* lValue = nullptr) override;
	virtual void ScopeLayout() override;
	virtual void Add(Expression* item) override;
	virtual bool Call(std::vector<Value>& params, Value& retValue);
};
class ExternFunc
	:public Func
{
	std::string m_funcName;
	U_FUNC m_func;
public:
	ExternFunc(std::string& funcName,U_FUNC func)
	{
		m_funcName = funcName;
		m_func = func;
	}
	virtual bool Call(std::vector<Value>& params,Value& retValue) override
	{
		return m_func ? m_func(params, retValue) : false;
	}
};
}}