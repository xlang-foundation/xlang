#include "package.h"
#include "manager.h"
#include "pyproxyobject.h"
#include "dotop.h"
#include "port.h"

extern std::string g_ExePath;

X::AST::Scope* X::AST::ScopeProxy::GetParentScope()
{
	return nullptr;
}
bool X::AST::Import::CalcCallables(Runtime* rt, void* pContext,
	std::vector<Scope*>& callables)
{
	ScopeProxy* pProxy = new ScopeProxy();
	callables.push_back(pProxy);
	return true;
}
bool X::AST::Import::FindAndLoadExtensions(std::string& curModulePath,
	std::string& loadingModuleName)
{
	std::string loadDllName;
	bool bHaveDll = false;
	//search xlang.exe folder first
	std::vector<std::string> candiateFiles;
	bool bRet = file_search(g_ExePath, loadingModuleName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	else
	{
		bRet = file_search(curModulePath, loadingModuleName + ShareLibExt, candiateFiles);
		if (bRet && candiateFiles.size() > 0)
		{
			loadDllName = candiateFiles[0];
			bHaveDll = true;
		}
	}
	bool bOK = false;
	if (bHaveDll)
	{
		typedef void (*LOAD)(void* pHost);
		void* libHandle = LOADLIB(loadDllName.c_str());
		if (libHandle)
		{
			LOAD load = (LOAD)GetProc(libHandle, "Load");
			if (load)
			{
				load((void*)g_pXHost);
			}
			bOK = true;
		}
	}
	return bOK;
}
bool X::AST::Import::Run(Runtime* rt, void* pContext, 
	Value& v, LValue* lValue)
{
	for (auto& im : m_importInfos)
	{
		std::string varName;
		if (im.alias.empty())
		{
			varName = im.name;
		}
		else
		{
			varName = im.alias;
		}
		//check if it is builtin
		if (m_path.empty())
		{
			Package* pPackage = nullptr;
			bool bOK = Manager::I().QueryAndCreatePackage(rt,
				im.name, &pPackage);
			if (bOK)
			{
				v = Value(pPackage);
				rt->M()->Add(rt, varName, nullptr, v);
				continue;
			}
		}
		else
		{
			Package* pPackage = nullptr;
			std::string curPath = rt->M()->GetModulePath();
			bool bLoaded = FindAndLoadExtensions(curPath, m_path);
			bool bOK = Manager::I().QueryAndCreatePackage(rt,
				im.name, &pPackage);
			if (bOK)
			{
				v = Value(pPackage);
				rt->M()->Add(rt, varName, nullptr, v);
				continue;
			}
		}
		//TODO: check if it is X module
		//
		//then Python Module
		std::string curPath = rt->M()->GetModulePath();
		auto* pProxyObj =
			new Data::PyProxyObject(rt, pContext,
				im.name,m_path, curPath);
		v = Value(pProxyObj);
		rt->M()->Add(rt, varName, nullptr, v);
	}
	return true;
}
std::string X::AST::Import::ConvertDotSeqToString(
	X::AST::Expression* expr)
{
	DotOp* dotOp = dynamic_cast<DotOp*>(expr);
	auto L0 = dotOp->GetL();
	auto R0 = dotOp->GetR();
	std::string leftName;
	std::string rightName;
	if (L0 && L0->m_type == ObType::Var)
	{
		leftName = ((Var*)L0)->GetNameString();
	}
	if (R0)
	{
		if (R0->m_type == ObType::Dot)
		{
			rightName = ConvertDotSeqToString(R0);
		}
		else if (R0->m_type == ObType::Var)
		{
			rightName = ((Var*)R0)->GetNameString();
		}
	}
	return leftName + "." + rightName;
};

void X::AST::Import::ScopeLayout()
{
	BinaryOp::ScopeLayout();
	if (L)
	{
		if (L->m_type == ObType::From)
		{
			m_path = ((From*)L)->GetPath();
		}
	}
	auto proc_AsOP = [&](Expression* expr)
	{
		AsOp* asOp = dynamic_cast<AsOp*>(expr);
		auto L0 = asOp->GetL();
		auto R0 = asOp->GetR();
		std::string leftName;
		std::string rightName;
		if (L0)
		{
			if (L0->m_type == ObType::Var)
			{
				leftName = ((Var*)L0)->GetNameString();
			}
			else if (L0->m_type == ObType::Dot)
			{
				leftName = ConvertDotSeqToString(L0);
			}
		}
		if (R0 && R0->m_type == ObType::Var)
		{
			rightName = ((Var*)R0)->GetNameString();
		}
		ImportInfo importInfo;
		importInfo.name = leftName;
		importInfo.alias = rightName;
		m_importInfos.push_back(importInfo);
	};
	auto proc_Var = [&](Expression* expr)
	{
		auto name = ((Var*)expr)->GetNameString();
		ImportInfo importInfo;
		importInfo.name = name;
		m_importInfos.push_back(importInfo);
	};
	auto proc_List = [&](Expression* expr)
	{
		List* pList = dynamic_cast<List*>(expr);
		auto list = pList->GetList();
		for (auto expr0 : list)
		{
			switch (expr0->m_type)
			{
			case ObType::As:
				proc_AsOP(expr0);
				break;
			case ObType::Var:
				proc_Var(expr0);
				break;
			default:
				break;
			}
		}
	};
	if (R)
	{//{var|AsOp}*
		switch (R->m_type)
		{
		case ObType::List:
			proc_List(R);
			break;
		case ObType::As:
			proc_AsOP(R);
			break;
		case ObType::Var:
			proc_Var(R);
			break;
		default:
			break;
		}
	}
}
bool X::AST::From::Run(Runtime* rt, void* pContext,
	Value& v, LValue* lValue)
{
	return true;
}
