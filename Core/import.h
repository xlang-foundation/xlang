#pragma once
#include "exp.h"
#include "object.h"
#include "stackframe.h"
#include "dotop.h"
#include "var.h"

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
public:
	ScopeProxy() :
		Scope()
	{
	}
};

//AsOp direived from BinaryOp, because we consider case below
//import galaxy as g, number as np
//here we need to combin left side name with right side name put 
//into one operand
class AsOp :
	public BinaryOp
{
public:
	AsOp() :
		BinaryOp()
	{
		m_type = ObType::As;
	}
	AsOp(short op) :
		BinaryOp(op)
	{
		m_type = ObType::As;
	}
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands, int LeftTokenIndex) override
	{
		//pop all non-var operands, store back
		//and in the end, push back again with same order
		std::vector<AST::Expression*> non_vars;
		//Find first var operand, and as its right operand
		//during loop, put all non-var operands into non_vars
		while (!operands.empty())
		{
			auto operand0 = operands.top();
			operands.pop();
			if (operand0->m_type == ObType::Var)
			{
				SetR(operand0);
				break;
			}
			non_vars.push_back(operand0);
		}
		//Find second var or Deferred operand, and as its left operand
		//during loop,put all non-var or non-Deferred operands into non_vars
		while (!operands.empty())
		{
			auto operand0 = operands.top();
			operands.pop();
			if (operand0->m_type == ObType::Var || operand0->m_type == ObType::Deferred)
			{
				SetL(operand0);
				break;
			}
			non_vars.push_back(operand0);
		}
		operands.push(this);
		//push back non-var operands
		for (auto rit = non_vars.rbegin(); rit != non_vars.rend(); ++rit)
		{
			operands.push(*rit);
		}

		return true;
	}
};
//put deffered operator as a unary operator,
//it will accept left side var as its operand
//which means who deffered
class DeferredOP :
	public UnaryOp
{
public:
	DeferredOP() :
		UnaryOp()
	{
		m_type = ObType::Deferred;
	}
	DeferredOP(short op) :
		UnaryOp(op)
	{
		m_type = ObType::Deferred;
	}
	//eat left side var as its right operand
	//because it is a unary operator,only has right operand
	//and put itself into operands stack
	virtual bool OpWithOperands(
		std::stack<AST::Expression*>& operands,
		int LeftTokenIndex)
	{
		if (!operands.empty())
		{
			auto operand0 = operands.top();
			if (operand0->m_type == ObType::Var)
			{
				operands.pop();
				SetR(operand0);
			}
		}
		operands.push(this);
		return true;
	}
};
class ThruOp :
	public UnaryOp
{
	std::string m_url;

	std::string CalcUrl(XlangRuntime* rt, XObj* pContext,Expression* l, Expression* r)
	{
		std::string l_name;
		std::string r_name;
		if (l)
		{
			if (l->m_type == ObType::Var || l->m_type == ObType::Str)
			{
				Value v0;
				ExecAction action;
				if (ExpExec(l,rt,action, pContext, v0, nullptr))
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
				ExecAction action;
				if (ExpExec(r,rt,action, pContext, v0, nullptr))
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
		UnaryOp()
	{
		m_type = ObType::Thru;
	}
	ThruOp(short op) :
		UnaryOp(op)
	{
		m_type = ObType::Thru;
	}
	std::string GetUrl() { return m_url; }
	bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext,
		Value& v, LValue* lValue) override
	{
		//Calc Path
		if (R)
		{
			if (R->m_type == ObType::Str || R->m_type == ObType::Var)
			{
				Value v0;
				if (ExpExec(R,rt,action, pContext, v0, nullptr))
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
	public UnaryOp
{
	std::string m_path;
public:
	From() :
		UnaryOp()
	{
		m_type = ObType::From;
	}
	From(short op) :
		UnaryOp(op)
	{
		m_type = ObType::From;
	}
	std::string GetPath()
	{
		return m_path;
	}
	bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext,
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
			else if (R->m_type == ObType::Dot)
			{
				m_path = CalcPathFromDotOp(R);
			}
		}
		v = Value(m_path);
		return true;
	}
	std::string CalcPathFromDotOp(Expression* dotExpr)
	{//for python case: folder.subfolder.module
		auto* pDotOp = dynamic_cast<AST::DotOp*>(dotExpr);
		auto* l = pDotOp->GetL();
		auto* r = pDotOp->GetR();
		std::string path;
		if (l)
		{
			std::string strL;
			if (l->m_type == ObType::Var)
			{
				strL = dynamic_cast<AST::Var*>(l)->GetNameString();
				path = strL;
			}
			else if (l->m_type == ObType::Dot)
			{
				strL = CalcPathFromDotOp(l);
				path = strL;
			}
		}
		if (r)
		{
			std::string strR;
			if (r->m_type == ObType::Var)
			{
				strR = dynamic_cast<AST::Var*>(r)->GetNameString();
				path = path + "." + strR;
			}
			else if (r->m_type == ObType::Dot)
			{
				strR = CalcPathFromDotOp(r);
				path = path + "." + strR;
			}
		}
		return path;
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
	bool Deferred = false;
};
class Import :
	public Operator
{
	From* m_from = nullptr;
	ThruOp* m_thru = nullptr;
	Expression* m_imports = nullptr;
	//from path import moudule_lists
	//only put one path after term: from
	std::string m_path;
	std::string m_thruUrl;
	std::vector<ImportInfo> m_importInfos;
	bool FindAndLoadExtensions(XlangRuntime* rt,
		std::string& curModulePath, std::string& loadingModuleName);
	bool FindAndLoadXModule(XlangRuntime* rt,
		std::string& curModulePath, std::string& loadingModuleName,
		Module** ppSubModule);
	bool LoadOneModule(XlangRuntime* rt, Scope* pMyScope,
		XObj* pContext, Value& v, ImportInfo& im, std::string& varNameForChange);
public:
	virtual bool ToBytes(XlangRuntime* rt, XObj* pContext, X::XLangStream& stream) override
	{
		Operator::ToBytes(rt, pContext, stream);
		stream << m_path;
		stream << m_thruUrl;
		stream << m_importInfos.size();	
		for (auto& i : m_importInfos)
		{
			stream << i.type;
			stream << i.name;
			stream << i.alias;
			stream << i.fileName;
			stream << i.Deferred;
		};

		return true;
	}
	virtual bool FromBytes(X::XLangStream& stream) override
	{
		Operator::FromBytes(stream);
		stream >> m_path;
		stream >> m_thruUrl;
		size_t size = 0;
		stream >> size;
		for (size_t i = 0; i < size; i++)
		{
			ImportInfo im;
			stream >> im.type;
			stream >> im.name;
			stream >> im.alias;
			stream >> im.fileName;
			stream >> im.Deferred;
			m_importInfos.push_back(im);
		}
		return true;
	}
	FORCE_INLINE From* GetFrom()
	{
		return m_from;
	}
	FORCE_INLINE ThruOp* GetThru()
	{
		return m_thru;
	}
	FORCE_INLINE Expression* GetImports()
	{
		return m_imports;
	}
	FORCE_INLINE ImportInfo* FindMatchedImportInfo(AST::ImportInfo& importInfo)
	{
		for (auto& im : m_importInfos)
		{
			if (im.type == importInfo.type &&
				im.name == importInfo.name &&
				im.alias == importInfo.alias &&
				im.fileName == importInfo.fileName &&
				im.Deferred == importInfo.Deferred)
			{
				return &im;
			}
		}
		return nullptr;
	}
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
	FORCE_INLINE bool LoadModule(XlangRuntime* rt,XObj* pContext,Value& v,ImportInfo* pImportInfo)
	{
		Scope* pMyScope = GetScope();
		std::string varName = pImportInfo->alias.empty() ? pImportInfo->name : pImportInfo->alias;
		return LoadOneModule(rt,pMyScope,pContext, v, *pImportInfo, varName);
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
		std::stack<AST::Expression*>& operands, int LeftTokenIndex)
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
				|| expr->m_type == ObType::As
				|| expr->m_type == ObType::Deferred)
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
	virtual bool Exec(XlangRuntime* rt,ExecAction& action, XObj* pContext,
		Value& v, LValue* lValue = nullptr) override;
	virtual bool CalcCallables(XlangRuntime* rt, XObj* pContext,
		std::vector<Scope*>& callables) override;
};
}
}