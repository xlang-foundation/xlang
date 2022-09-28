#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"

namespace X 
{ 
namespace AST
{
class ScopeProxy :
	public Scope
{
	// Inherited via Scope
	virtual AST::ScopeWaitingStatus IsWaitForCall() override
	{
		return AST::ScopeWaitingStatus::HasWaiting;
	}
	virtual Scope* GetParentScope() override;
public:
	ScopeProxy() :
		Scope()
	{
	}
};
class AsOp :
	virtual public BinaryOp
{
public:
	AsOp() :
		Operator(),
		BinaryOp()
	{
		m_type = ObType::As;
	}
	AsOp(short op) :
		Operator(op),
		BinaryOp(op)
	{
		m_type = ObType::As;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		//pop all non-var operands, store back
		//and in the end, push back again with same order
		std::vector<AST::Expression*> non_vars;
		while (!operands.empty())
		{
			auto operand0 = operands.top();
			if (operand0->m_type == ObType::Var)
			{
				break;
			}
			operands.pop();
			non_vars.push_back(operand0);
		}
		if (!operands.empty())
		{
			auto operandR = operands.top();
			operands.pop();
			SetR(operandR);
		}
		if (!operands.empty())
		{
			auto operandL = operands.top();
			operands.pop();
			SetL(operandL);
		}
		operands.push(this);
		for (auto rit = non_vars.rbegin(); rit != non_vars.rend(); ++rit)
		{
			operands.push(*rit);
		}

		return true;
	}
};
class ThruOp :
	virtual public UnaryOp
{
	std::string m_url;

	std::string CalcUrl(Runtime* rt, XObj* pContext,Expression* l, Expression* r)
	{
		std::string l_name;
		std::string r_name;
		if (l)
		{
			if (l->m_type == ObType::Var || l->m_type == ObType::Str)
			{
				Value v0;
				if (l->Run(rt, pContext, v0, nullptr))
				{
					l_name = v0.ToString();
				}
			}
			else if (l->m_type == ObType::BinaryOp)
			{
				l_name = CalcUrl(rt,pContext,(dynamic_cast<BinaryOp*>(l))->GetL(),
					(dynamic_cast<BinaryOp*>(l))->GetR());
			}
		}
		if (r)
		{
			if (r->m_type == ObType::Var)
			{
				Value v0;
				if (r->Run(rt, pContext, v0, nullptr))
				{
					r_name = v0.ToString();
				}
			}
			else if (r->m_type == ObType::BinaryOp)
			{
				r_name = CalcUrl(rt, pContext, (dynamic_cast<BinaryOp*>(r))->GetL(),
					(dynamic_cast<BinaryOp*>(r))->GetR());
			}
		}
		return l_name + "/" + r_name;
	}
public:
	ThruOp() :
		Operator(),
		UnaryOp()
	{
		m_type = ObType::Thru;
	}
	ThruOp(short op) :
		Operator(op),
		UnaryOp(op)
	{
		m_type = ObType::Thru;
	}
	std::string GetUrl() { return m_url; }
	bool Run(Runtime* rt, XObj* pContext,
		Value& v, LValue* lValue) override
	{
		//Calc Path
		if (R)
		{
			if (R->m_type == ObType::Str || R->m_type == ObType::Var)
			{
				Value v0;
				if (R->Run(rt, pContext, v0, nullptr))
				{
					m_url = v0.ToString();
				}
			}
			else if (R->m_type == ObType::BinaryOp)
			{
				m_url = CalcUrl(rt, pContext, (dynamic_cast<BinaryOp*>(R))->GetL(),
					(dynamic_cast<BinaryOp*>(R))->GetR());
			}
		}
		v = Value(m_url);
		return true;
	}
};
class From :
	virtual public UnaryOp
{
	std::string m_path;
public:
	From() :
		Operator(),
		UnaryOp()
	{
		m_type = ObType::From;
	}
	From(short op) :
		Operator(op),
		UnaryOp(op)
	{
		m_type = ObType::From;
	}
	std::string GetPath()
	{
		return m_path;
	}
	bool Run(Runtime* rt, XObj* pContext,
		Value& v, LValue* lValue) override
	{
		//Calc Path
		//format like folder1/folder2/.../folderN
		if (R)
		{
			if (R->m_type == ObType::Var)
			{
				m_path = (dynamic_cast<Var*>(R))->GetNameString();
			}
			else if (R->m_type == ObType::BinaryOp)
			{
				m_path = CalcPath((dynamic_cast<BinaryOp*>(R))->GetL(),
					(dynamic_cast<BinaryOp*>(R))->GetR());
			}
		}
		v = Value(m_path);
		return true;
	}
	std::string CalcPath(Expression* l, Expression* r)
	{
		std::string l_name;
		std::string r_name;
		if (l)
		{
			if (l->m_type == ObType::Var)
			{
				l_name = (dynamic_cast<Var*>(l))->GetNameString();
			}
			else if (l->m_type == ObType::BinaryOp)
			{
				l_name = CalcPath((dynamic_cast<BinaryOp*>(l))->GetL(),
					(dynamic_cast<BinaryOp*>(l))->GetR());
			}
		}
		if (r)
		{
			if (r->m_type == ObType::Var)
			{
				r_name = (dynamic_cast<Var*>(r))->GetNameString();
			}
			else if (r->m_type == ObType::BinaryOp)
			{
				r_name = CalcPath((dynamic_cast<BinaryOp*>(r))->GetL(),
					(dynamic_cast<BinaryOp*>(r))->GetR());
			}
		}
		return l_name + "/" + r_name;
	}
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
	virtual public Operator
{
	From* m_from = nullptr;
	ThruOp* m_thru = nullptr;
	Expression* m_imports = nullptr;
	//from path import moudule_lists
	//only put one path after term: from
	std::string m_path;
	std::string m_thruUrl;
	std::vector<ImportInfo> m_importInfos;
	bool FindAndLoadExtensions(Runtime* rt,
		std::string& curModulePath, std::string& loadingModuleName);
	bool FindAndLoadXModule(Runtime* rt,
		std::string& curModulePath, std::string& loadingModuleName,
		Module** ppSubModule);
public:
	Import() :
		Operator()
	{
		m_type = ObType::Import;
	}
	Import(const char* moduleName,
		const char* from=nullptr, const char* thru=nullptr):
		Operator()
	{
		m_type = ObType::Import;
		if (moduleName)
		{
			std::string strModuleName = moduleName;
			m_importInfos.push_back(ImportInfo{ ImportType::Builtin,strModuleName });
		}
		if (from)
		{
			m_path = from;
		}
		if (thru)
		{
			m_thruUrl = thru;
		}
	}
	Import(short op) :
		Operator(op)
	{
		m_type = ObType::Import;
	}
	virtual int GetLeftMostCharPos() override
	{
		int pos = GetCharPos();
		int startLine = GetStartLine();
		if (m_from)
		{
			int posL = m_from->GetLeftMostCharPos();
			if (posL < pos && m_from->GetStartLine() <= startLine)
			{
				pos = posL;
			}
		}
		if (m_imports)
		{
			int posR = m_imports->GetLeftMostCharPos();
			if (posR < pos && m_imports->GetStartLine() <= startLine)
			{
				pos = posR;
			}
		}
		return pos;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands)
	{
		if (!operands.empty())
		{
			auto expr = operands.top();
			if (expr->m_type == ObType::Thru)
			{
				m_thru = dynamic_cast<ThruOp*>(expr);
				operands.pop();
			}
		}
		if (!operands.empty())
		{
			auto expr = operands.top();
			if (expr->m_type == ObType::Var 
				||expr->m_type == ObType::List
				|| expr->m_type == ObType::As)
			{
				m_imports = expr;
				operands.pop();
			}
		}
		if (!operands.empty())
		{
			auto expr = operands.top();
			if (expr->m_type == ObType::From)
			{
				m_from = dynamic_cast<From*>(expr);
				operands.pop();
			}
		}
		operands.push(this);
		return true;
	}
	std::string ConvertDotSeqToString(Expression* expr);
	virtual void ScopeLayout() override;
	virtual bool Run(Runtime* rt, XObj* pContext,
		Value& v, LValue* lValue = nullptr) override;
	virtual bool CalcCallables(Runtime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;
};
}
}