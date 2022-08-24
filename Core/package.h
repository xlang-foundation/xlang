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
	// Inherited via Scope
	virtual AST::ScopeWaitingStatus IsWaitForCall() override
	{
		return AST::ScopeWaitingStatus::HasWaiting;
	}
	virtual Scope* GetParentScope() override;
public:
	ScopeProxy():
		Scope()
	{
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
	bool FindAndLoadExtensions(Runtime* rt,
		std::string& curModulePath,std::string& loadingModuleName);
	bool FindAndLoadXModule(Runtime* rt,
		std::string& curModulePath, std::string& loadingModuleName,
		Module** ppSubModule);
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
	virtual public XPackage,
	virtual public Data::Object,
	virtual public Scope
{
	void* m_pObject = nullptr;
	StackFrame* m_stackFrame = nullptr;
public:
	Package(void* pObj):
		Data::Object(), Scope()
	{
		m_pObject = pObj;
		m_t = X::ObjType::Package;
	}
	~Package()
	{
		std::cout << "~Package()"<<std::endl;
	}
	virtual int AddMethod(const char* name) override
	{
		std::string strName(name);
		return Scope::AddOrGet(strName, false);
	}
	inline virtual AST::Scope* GetScope()
	{
		return dynamic_cast<Scope*>(this);
	}
	virtual void* GetEmbedObj() override 
	{ 
		return m_pObject; 
	}
	virtual bool Init(int varNum) override
	{
		m_stackFrame = new StackFrame(this);
		m_stackFrame->SetVarCount(varNum);
		return true;
	}
	virtual bool Call(XRuntime* rt, ARGS& params,
		KWARGS& kwParams,
		X::Value& retValue)
	{
		return true;
	}
	// Inherited via Scope
	virtual bool Set(Runtime* rt, void* pContext, int idx, Value& v) override
	{
		m_stackFrame->Set(idx, v);
		return true;
	}
	virtual bool SetIndexValue(XRuntime* rt, void* pContext, int idx, Value& v) override
	{
		return Set((Runtime*)rt, pContext, idx, v);
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