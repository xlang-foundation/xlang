#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"

namespace X
{
namespace AST
{
class ScopeProxy:
	public Scope
{
	std::string m_name;
	virtual bool isEqual(Scope* s) override;

	// Inherited via Scope
	virtual AST::ScopeWaitingStatus IsWaitForCall() override
	{
		return AST::ScopeWaitingStatus::NeedFurtherCallWithName;
	}
	virtual AST::ScopeWaitingStatus IsWaitForCall(std::string& name) override
	{
		m_name = name;
		return AST::ScopeWaitingStatus::HasWaiting;
	};
	virtual Scope* GetParentScope() override;
public:
	ScopeProxy(std::string name):
		Scope()
	{
		m_name = name;
	}
};
class AsOp :
	public BinaryOp
{
public:
	AsOp(short op) :
		BinaryOp(op)
	{
		m_type = ObType::As;
	}
};
class From :
	public UnaryOp
{
	std::string m_path;
public:
	From(short op) :
		UnaryOp(op)
	{
		m_type = ObType::From;
	}
	std::string GetPath()
	{
		return m_path;
	}
	std::string CalcPath(Expression* l, Expression* r)
	{
		std::string l_name;
		std::string r_name;
		if (l)
		{
			if (l->m_type == ObType::Var)
			{
				l_name = ((Var*)l)->GetNameString();
			}
			else if (l->m_type == ObType::BinaryOp)
			{				
				l_name = CalcPath(((BinaryOp*)l)->GetL(),
					((BinaryOp*)l)->GetR());
			}
		}
		if (r)
		{
			if (r->m_type == ObType::Var)
			{
				r_name = ((Var*)r)->GetNameString();
			}
			else if (r->m_type == ObType::BinaryOp)
			{
				r_name = CalcPath(((BinaryOp*)r)->GetL(),
					((BinaryOp*)r)->GetR());
			}
		}
		return l_name + "/" + r_name;
	}
	virtual void ScopeLayout() override
	{
		UnaryOp::ScopeLayout();
		//Calc Path
		//format like folder1/folder2/.../folderN
		if (R)
		{
			if (R->m_type == ObType::Var)
			{
				m_path = ((Var*)R)->GetNameString();
			}
			else if (R->m_type == ObType::BinaryOp)
			{
				m_path = CalcPath(((BinaryOp*)R)->GetL(),
					((BinaryOp*)R)->GetR());
			}
		}
	}
	virtual bool Run(Runtime* rt, void* pContext,
		Value& v, LValue* lValue = nullptr) override;
};

enum class ImportType
{
	Builtin,
	XModule,
	PyModule,
};

struct ImportInfo
{
	ImportType type = ImportType::Builtin;
	std::string name;
	std::string alias;
	std::string fileName;
};
class Import :
	public BinaryOp
{
	//from path import moudule_lists
	//only put one path after term: from
	std::string m_path;

	std::vector<ImportInfo> m_importInfos;
public:
	Import(short op) :
		BinaryOp(op)
	{
		m_type = ObType::Import;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		auto operandR = operands.top();
		operands.pop();
		SetR(operandR);
		if (!operands.empty())
		{//from can be igored
			auto operandL = operands.top();
			operands.pop();
			SetL(operandL);
		}
		operands.push(this);
		return true;
	}
	std::string ConvertDotSeqToString(Expression* expr);
	virtual void ScopeLayout() override;
	virtual bool Run(Runtime* rt, void* pContext,
		Value& v, LValue* lValue = nullptr) override;
	virtual bool CalcCallables(Runtime* rt, void* pContext,
		std::vector<Scope*>& callables) override;
};
class Package :
	public Data::Object,
	public Scope
{
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = Data::Type::Package;
	}
	~Package()
	{
		std::cout << "~Package()"<<std::endl;
	}
	inline virtual AST::Scope* GetScope()
	{
		return dynamic_cast<Scope*>(this);
	}
	void* GetObj() { return m_pObject; }
	bool Init(int varNum)
	{
		m_stackFrame = new StackFrame(this);
		m_stackFrame->SetVarCount(varNum);
		return true;
	}
	virtual bool Call(Runtime* rt, ARGS& params,
		KWARGS& kwParams,
		AST::Value& retValue)
	{
		return true;
	}

	// Inherited via Scope
	virtual bool Set(Runtime* rt, void* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool Get(Runtime* rt, void* pContext, int idx, Value& v,
		LValue* lValue = nullptr) override
	{
		m_stackFrame->Get(idx, v, lValue);
		return true;
	}
	virtual Scope* GetParentScope() override
	{
		return nullptr;
	}
};
}
}