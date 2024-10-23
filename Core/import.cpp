/*
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

#include "import.h"
#include "manager.h"
#include "pyproxyobject.h"
#include "dotop.h"
#include "port.h"
#include "Hosting.h"
#include "moduleobject.h"
#include "remote_object.h"
#include "deferred_object.h"
#include "op.h"
#include "../Jit/md5.h"
#include <filesystem>

namespace X
{
	extern XLoad* g_pXload;
	extern bool LoadPythonEngine();

}

bool X::AST::Import::CalcCallables(XlangRuntime* rt, XObj* pContext,
	std::vector<Scope*>& callables)
{
	ScopeProxy* pProxy = new ScopeProxy();
	callables.push_back(pProxy);
	return true;
}
bool X::AST::Import::FindAndLoadExtensions(XlangRuntime* rt,
	std::string& curModulePath,
	std::string& loadingModuleName)
{
	std::string loadDllName;
	bool bHaveDll = false;
	//search xlang.app folder first
	std::vector<std::string> candiateFiles;
	bool bRet = file_search(g_pXload->GetConfig().appPath,
		LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
	if (bRet && candiateFiles.size() > 0)
	{
		loadDllName = candiateFiles[0];
		bHaveDll = true;
	}
	//search xlang.engine folder first
	if (!bHaveDll)
	{
		bRet = file_search(g_pXload->GetConfig().xlangEnginePath,
			LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
		if (bRet && candiateFiles.size() > 0)
		{
			loadDllName = candiateFiles[0];
			bHaveDll = true;
		}
	}
	//check dll search path
	if (!bHaveDll && g_pXload->GetConfig().dllSearchPath)
	{
		std::string dllSearchPath(g_pXload->GetConfig().dllSearchPath);
		std::vector<std::string> paths = split(dllSearchPath, '\n');
		for (auto& p : paths)
		{
			bRet = file_search(p, LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	if (!bHaveDll && !curModulePath.empty())
	{
		bRet = file_search(curModulePath, LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
		if (bRet && candiateFiles.size() > 0)
		{
			loadDllName = candiateFiles[0];
			bHaveDll = true;
		}
	}
	if (!bHaveDll)
	{
		std::vector<std::string> searchPaths;
		rt->M()->GetSearchPaths(searchPaths);
		for (auto& pa : searchPaths)
		{
			bRet = file_search(pa, LibPrefix + loadingModuleName + ShareLibExt, candiateFiles);
			if (bRet && candiateFiles.size() > 0)
			{
				loadDllName = candiateFiles[0];
				bHaveDll = true;
				break;
			}
		}
	}
	bool bOK = false;
	if (bHaveDll)
	{
		typedef void (*LOAD)(void* pHost, X::Value module);
		void* libHandle = LOADLIB(loadDllName.c_str());
		if (libHandle)
		{
			LOAD load = (LOAD)GetProc(libHandle, "Load");
			if (load)
			{
				ModuleObject* pModuleObj = new ModuleObject(rt->M());
				Value curModule = Value(pModuleObj);
				load((void*)g_pXHost, curModule);
			}
			bOK = true;
		}
	}
	return bOK;
}

bool endsWithDotX(const std::string& str) 
{
	return str.length() >= 2 &&
		str[str.length() - 2] == '.' &&
		std::tolower(str[str.length() - 1]) == 'x';
}


bool X::AST::Import::FindAndLoadXModule(XlangRuntime* rt,
	std::string& curModulePath,
	std::string& loadingModuleName,
	Module** ppSubModule)
{
	std::string loadXModuleFileName;
	bool bHaveX = false;
	std::string prefixPath;
	if (!m_path.empty())
	{
		//we check m_path if it is a full path of .x file
		bHaveX = endsWithDotX(m_path);
		if (bHaveX)
		{
			loadXModuleFileName = m_path;
		}
		else
		{
			prefixPath = m_path;
			ReplaceAll(prefixPath, ".", Path_Sep_S);
		}
	}
	if (!bHaveX)
	{
		std::vector<std::string> searchPaths;
		searchPaths.push_back(curModulePath);
		searchPaths.push_back(g_pXload->GetConfig().xlangEnginePath);

		auto search = [](std::string& loadingModuleName,
			std::vector<std::string>& searchPaths,
			std::string& loadXModuleName, std::string prefixPath)
			{
				for (auto& pa : searchPaths)
				{
					std::vector<std::string> candiateFiles;
					bool bRet = file_search(pa + (prefixPath.empty() ? "" : Path_Sep_S + prefixPath),
						loadingModuleName + ".x", candiateFiles);
					if (bRet && candiateFiles.size() > 0)
					{
						loadXModuleName = candiateFiles[0];
						return true;
					}
				}
				return false;
			};
		bHaveX = search(loadingModuleName, searchPaths, loadXModuleFileName, prefixPath);
		if (!bHaveX)
		{
			rt->M()->GetSearchPaths(searchPaths);
			bHaveX = search(loadingModuleName, searchPaths, loadXModuleFileName, prefixPath);
		}
	}
	bool bOK = false;
	if (bHaveX)
	{
		std::string code;
		std::filesystem::path pathModuleName(loadXModuleFileName);
		loadXModuleFileName = pathModuleName.generic_string();

		bOK = LoadStringFromFile(loadXModuleFileName, code);
		if (bOK)
		{
			unsigned long long moduleKey = 0;
			auto* pSubModule = Hosting::I().Load(loadXModuleFileName.c_str(),
				code.c_str(), (int)code.size(), moduleKey, md5(code));
			if (pSubModule)
			{
				X::Value v0;
				std::vector<X::Value> passInParams;
				bOK = Hosting::I().Run(pSubModule, v0, passInParams);
			}
			*ppSubModule = pSubModule;
		}
	}
	return bOK;
}

bool X::AST::Import::Exec(XlangRuntime* rt, ExecAction& action, XObj* pContext,
	Value& v, LValue* lValue)
{
	Scope* pMyScope = GetScope();
	if (m_from)
	{
		Value v0;
		if (ExpExec(m_from,rt, action, pContext, v0, nullptr))
		{
			m_path = v0.ToString();
		}
	}
	if (m_thru)
	{
		Value v0;
		if (ExpExec(m_thru,rt, action, pContext, v0, nullptr))
		{
			m_thruUrl = v0.ToString();
		}
	}
	for (auto& im : m_importInfos)
	{
		//for deferred object, create a DeferredObject object to wrap the import info
		//and set this object's value to current scope
		std::string varName = im.alias.empty() ? im.name : im.alias;
		bool bOK = false;
		if (im.Deferred)
		{
			auto* deferredObj = new Data::DeferredObject();
			deferredObj->SetImportInfo(this, &im);
			v = Value(dynamic_cast<XObj*>(deferredObj));
			bOK = true;
		}
		else
		{
			bOK = LoadOneModule(rt, pMyScope, pContext, v, im, varName);
		}
		if (bOK && pMyScope)
		{
			SCOPE_FAST_CALL_AddOrGet0(idx,pMyScope,varName, false);
			rt->Set(pMyScope, pContext,idx,v);
		}
	}
	return true;
}
bool X::AST::Import::ExpRun(XlangRuntime* rt, X::XObj* pContext, X::Exp::ValueStack& valueStack, X::Value& retValue)
{
	Scope* pMyScope = GetScope();
	if (m_from)
	{
		m_path = valueStack.top().v.ToString();
		valueStack.pop();
	}
	if (m_thru)
	{
		m_thruUrl = valueStack.top().v.ToString();
		valueStack.pop();
	}
	for (auto& im : m_importInfos)
	{
		//for deferred object, create a DeferredObject object to wrap the import info
		//and set this object's value to current scope
		std::string varName = im.alias.empty() ? im.name : im.alias;
		bool bOK = false;
		if (im.Deferred)
		{
			auto* deferredObj = new Data::DeferredObject();
			deferredObj->SetImportInfo(this, &im);
			retValue = Value(dynamic_cast<XObj*>(deferredObj));
			bOK = true;
		}
		else
		{
			bOK = LoadOneModule(rt, pMyScope, pContext, retValue, im, varName);
		}
		if (bOK && pMyScope)
		{
			SCOPE_FAST_CALL_AddOrGet0(idx, pMyScope, varName, false);
			rt->Set(pMyScope, pContext, idx, retValue);
		}
	}

	return true;
}

bool X::AST::Import::LoadOneModule(XlangRuntime* rt, Scope* pMyScope,
	XObj* pContext, Value& v, ImportInfo& im, std::string& varNameForChange)
{
	if (!m_thruUrl.empty())
	{
		bool bFind = Manager::I().QueryPackage(im.name, v);
		if (bFind)
		{
			return true;
		}
		bool bFilterOut = false;
		XProxy* proxy = Manager::I().QueryProxy(m_thruUrl, bFilterOut);
		if (!bFilterOut)
		{
			if (proxy)
			{
				proxy->SetRootObjectName(im.name.c_str());
				auto* remoteObj = new RemoteObject(proxy);
				remoteObj->SetObjName(im.name);
				//todo: need to check here
				v = Value(dynamic_cast<XObj*>(remoteObj));
				Manager::I().Register(im.name.c_str(), v);
				return true;
			}
			//for proxy == nullptr, means some errors happened with this url
			return false;
		}
	}
	//then try local import
	if (Manager::I().QueryAndCreatePackage(rt, im.name, v))
	{
		return true;
	}

	//check if it is builtin
	if (m_path.empty())
	{
		if (Manager::I().QueryAndCreatePackage(rt, im.name, v))
		{
			return true;
		}
	}
	else
	{
		std::string curPath;
		if (rt->M())
		{
			curPath = rt->M()->GetModulePath();
		}
		bool bLoaded = FindAndLoadExtensions(rt, curPath, m_path);
		if (bLoaded)
		{
			if (Manager::I().QueryAndCreatePackage(rt, im.name, v))
			{
				return true;
			}
		}
	}
	//Check if it is X module
	std::string curPath = rt->M()->GetModulePath();
	Module* pSubModule = nullptr;
	bool bOK = FindAndLoadXModule(rt, curPath, im.name, &pSubModule);
	if (bOK && pSubModule != nullptr)
	{
		ModuleObject* pModuleObj = new ModuleObject(pSubModule);
		v = Value(pModuleObj);
		return true;
	}
	//then try Python Module
	//reserved import for python builtins, such as open, close etc
	//import python
	//if get this import, if enablePython is false, 
	//change to true and Load Python engine
	std::string moduleName = im.name;
	if (m_path.empty() && moduleName == "python")
	{
		if (!g_pXload->GetConfig().enablePython)
		{
			g_pXload->GetConfig().enablePython = true;
			LoadPythonEngine();
		}
		moduleName = "builtins";
		varNameForChange = "python";
	}
	if (g_pXload->GetConfig().enablePython)
	{
		auto* pProxyObj =
			new Data::PyProxyObject(rt, pContext,
				moduleName, m_path, curPath);
		v = Value(pProxyObj);
		return true;
	}

	return false;
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
		leftName = (dynamic_cast<Var*>(L0))->GetNameString();
	}
	if (R0)
	{
		if (R0->m_type == ObType::Dot)
		{
			rightName = ConvertDotSeqToString(R0);
		}
		else if (R0->m_type == ObType::Var)
		{
			rightName = (dynamic_cast<Var*>(R0))->GetNameString();
		}
	}
	return leftName + "." + rightName;
};

void X::AST::Import::ScopeLayout()
{
	Operator::ScopeLayout();
	auto* pMyScope = GetScope();
	if (m_from)
	{
		m_from->SetParent(m_parent);
		m_from->ScopeLayout();
	}
	auto proc_AsOP = [&](Expression* expr)
	{
		AsOp* asOp = dynamic_cast<AsOp*>(expr);
		AST::Expression* L0 = asOp->GetL();
		auto R0 = asOp->GetR();
		std::string leftName;
		std::string rightName;
		bool isDeferred = false;
		if (L0)
		{
			if (L0->m_type == ObType::Var)
			{
				leftName = (dynamic_cast<Var*>(L0))->GetNameString();
			}
			else if (L0->m_type == ObType::Deferred)
			{
				auto* pDeferred = dynamic_cast<DeferredOP*>(L0);
				if (pDeferred->GetR())
				{
					auto* pVar = dynamic_cast<Var*>(pDeferred->GetR());
					leftName = pVar->GetNameString();
					isDeferred = true;
				}
				else
				{
					//error
				}
			}
			else if (L0->m_type == ObType::Dot)
			{
				leftName = ConvertDotSeqToString(L0);
			}
		}
		if (R0 && R0->m_type == ObType::Var)
		{
			rightName = (dynamic_cast<Var*>(R0))->GetNameString();
		}
		ImportInfo importInfo;
		importInfo.name = leftName;
		importInfo.alias = rightName;
		importInfo.Deferred = isDeferred;
		m_importInfos.push_back(importInfo);
	};
	auto proc_Deferred = [&](Expression* expr)
	{
		auto* pDeferred = dynamic_cast<DeferredOP*>(expr);
		if (pDeferred->GetR())
		{
			auto name = (dynamic_cast<Var*>(pDeferred->GetR()))->GetNameString();
			ImportInfo importInfo;
			importInfo.name = name;
			importInfo.Deferred = true;
			m_importInfos.push_back(importInfo);
		}

	};
	auto proc_Var = [&](Expression* expr)
	{
		auto name = (dynamic_cast<Var*>(expr))->GetNameString();
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
			case ObType::Deferred:
				proc_Deferred(expr0);
				break;
			default:
				break;
			}
		}
	};
	if (m_imports)
	{//{var|AsOp}*
		switch (m_imports->m_type)
		{
		case ObType::List:
			proc_List(m_imports);
			break;
		case ObType::As:
			proc_AsOP(m_imports);
			break;
		case ObType::Var:
			proc_Var(m_imports);
			break;
		case ObType::Deferred:
			proc_Deferred(m_imports);
			break;
		default:
			break;
		}
	}
	if (m_thru)
	{
		m_thru->SetParent(m_parent);
		m_thru->ScopeLayout();
	}
}
