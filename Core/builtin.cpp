#include "builtin.h"
#include "exp.h"
#include "object.h"
#include "funclist.h"
#include "function.h"
#include <iostream>
#include "port.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <time.h> 
#include "utility.h"
#include "task.h"
#include <vector>
#include "Locker.h"
#include "def.h"
#include "Hosting.h"
#include <fstream>
#include <sstream>
#include "event.h"
#include "bin.h"
#include "BlockStream.h"
#include "xpackage.h"
#include "json.h"
#include "metascope.h"
#include "attribute.h"
#include "devops.h"

Locker _printLock;
bool U_Print(X::XRuntime* rt,void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	_printLock.Lock();
	for (auto& v : params)
	{
		std::string str = v.ToString();
		std::cout << str;
	}
	std::cout << std::endl;
	_printLock.Unlock();
	retValue = X::Value(true);
	return true;
}
bool U_Load(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string fileName = params[0].ToString();
	std::string ext = ExtName(fileName);
	if (ext != "X" && ext != "x")
	{
		retValue = X::Value(0);
		return true;
	}
	std::ifstream moduleFile(fileName);
	std::string code((std::istreambuf_iterator<char>(
		moduleFile)), std::istreambuf_iterator<char>());
	moduleFile.close();
	unsigned long long moduleKey = 0;
	X::Hosting::I().Load(fileName, code.c_str(), (int)code.size(), moduleKey);
	retValue = X::Value(moduleKey);
	return true;
}

bool U_Run(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	unsigned long long key = 0;
	if (params.size() > 0)
	{
		key = params[0].GetLongLong();
	}
	else
	{
		auto it = kwParams.find("ModuleKey");
		if (it != kwParams.end())
		{
			key = it->second.GetLongLong();
		}
	}
	return X::Hosting::I().Run(key, kwParams, retValue);
}
bool U_RunCode(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() < 2)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string moduleName = params[0].ToString();
	std::string code = params[1].ToString();
	return X::Hosting::I().Run(moduleName, code.c_str(), (int)code.size(), retValue);
}
bool U_RunInMain(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Event* pEvt = X::EventSystem::I().Query("RunModule");
	if (pEvt == nullptr)
	{
		pEvt = X::EventSystem::I().Register("RunModule");
		pEvt->Add([](void* pContext, void* pContext2, X::Event* pEvt)
			{
				unsigned long long mKey = 0;
				auto valKey = pEvt->Get("ModuleKey");
				mKey = valKey.GetLongLong();
				X::KWARGS kwParams0;
				pEvt->CovertPropsToArgs(kwParams0);
				X::Value retValue0;
				X::Hosting::I().Run(mKey, kwParams0, retValue0);
			}, rt,nullptr);
	}
	unsigned long long key = 0;
	if (params.size() > 0)
	{
		key = params[0].GetLongLong();
	}
	else
	{
		auto it = kwParams.find("ModuleKey");
		if (it != kwParams.end())
		{
			key = it->second.GetLongLong();
		}
	}
	X::Value valKey(key);
	pEvt->Set("ModuleKey", valKey);
	for (auto& it : kwParams)
	{
		pEvt->Set(it.first.c_str(), it.second);
	}
	pEvt->FireInMain();
	pEvt->Release();
	return true;
}

bool U_Rand(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	static bool init = false;
	if (!init)
	{
		srand((unsigned int)time(nullptr));
		init = true;
	}
	int r = rand();
	retValue = X::Value(r);
	return true;
}
bool U_ThreadId(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long id = GetThreadID();
	retValue = X::Value(id);
	return true;
}
bool U_Sleep(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	bool bOK = true;
	long long t = 0;
	if (pContext == nullptr)
	{
		if (params.size() > 0)
		{
			t = params[0].GetLongLong();
		}
		else
		{
			auto it = kwParams.find("time");
			if (it != kwParams.end())
			{
				t = it->second.GetLongLong();
			}
		}
	}
	else
	{//must put into kwargs with time=t
		auto it = kwParams.find("time");
		if (it != kwParams.end())
		{
			t = it->second.GetLongLong();
		}
	}
	MS_SLEEP((int)t);
	if (pContext)
	{//with a function, means after sleep, call this function
		X::Data::Function* pFuncObj = (X::Data::Function*)pContext;
		X::AST::Func* pFunc = pFuncObj->GetFunc();
		bOK = pFunc->Call(rt, nullptr,params, kwParams, retValue);
	}
	else
	{
		retValue = X::Value(bOK);
	}
	return bOK;
}
bool U_Time(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long t = getCurMilliTimeStamp();
	retValue = X::Value(t);
	return true;
}


namespace X {
	void Builtin::Cleanup()
	{
		for (auto& it : m_mapFuncs)
		{
			if (it.second)
			{
				delete it.second;
			}
		}
		m_mapFuncs.clear();
	}
	AST::ExternFunc* Builtin::Find(std::string& name)
{
	auto it = m_mapFuncs.find(name);
	return (it!= m_mapFuncs.end())?it->second:nil;
}
bool Builtin::Register(const char* name, void* func,
	std::vector<std::pair<std::string, std::string>>& params,
	bool regToMeta)
{
	std::string strName(name);
	AST::ExternFunc* extFunc = new AST::ExternFunc(
		strName,
		(X::U_FUNC)func);
	m_mapFuncs.emplace(std::make_pair(name,extFunc));
	if (regToMeta)
	{
		int idx = X::AST::MetaScope::I().AddOrGet(strName, false);
		auto* pFuncObj = new Data::Function(extFunc);
		pFuncObj->AddRef();
		X::Value vFunc(pFuncObj);
		X::AST::MetaScope::I().Set(nullptr, nullptr, idx,vFunc);
	}
	return true;
}

bool U_BreakPoint(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	KWARGS& kwParams,
	X::Value& retValue)
{
	((X::Runtime*)rt)->M()->SetDbgType(AST::dbg::Step,
		AST::dbg::Step);
	retValue = X::Value(true);
	return true;
}
bool U_ToString(X::XRuntime* rt, void* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() != 1)
	{
		retValue = X::Value(false);
		return false;
	}
	auto formatFlag = kwParams["format"];
	bool bFmt = formatFlag.IsTrue();
	auto retStr = params[0].ToString(bFmt);
	retValue = X::Value(retStr);
	return true;
}
bool U_ToBytes(X::XRuntime* rt, void* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	X::BlockStream stream;
	for (auto& v : params)
	{
		stream << v;
	}
	auto size = stream.Size();
	char* pData = new char[size];
	stream.FullCopyTo(pData, size);
	X::Data::Binary* pBinOut = new X::Data::Binary(pData, size);
	retValue = X::Value(pBinOut);
	return true;
}

bool U_TaskRun(X::XRuntime* rt, void* pContext,
	ARGS& params,KWARGS& kwParams,
	X::Value& retValue)
{
	Data::List* pFutureList = nil;
	Data::Future* pFuture = nil;
	auto buildtask = [&](X::AST::Func* pFunc)
	{
		X::Task* tsk = new X::Task();
		Data::Future* f = new Data::Future(tsk);
		if (pFuture || pFutureList)
		{
			if (pFutureList == nil)
			{
				pFutureList = new Data::List();
				X::Value vTask(pFuture);
				pFutureList->Add((X::Runtime*)rt, vTask);
				pFuture = nil;
			}
			X::Value vTask(f);
			pFutureList->Add((X::Runtime*)rt, vTask);
		}
		else
		{
			pFuture = f;
		}
		bool bRet = tsk->Call(pFunc, (X::Runtime*)rt, pContext, params, kwParams);
		return bRet;
	};
	bool bOK = true;
	auto* pContextObj = (X::Data::Object*)pContext;
	if (pContextObj->GetType() == X::ObjType::FuncCalls)
	{
		X::Data::FuncCalls* pFuncCalls = (X::Data::FuncCalls*)pContextObj;
		auto& list = pFuncCalls->GetList();
		for (auto& i : list)
		{
			buildtask(i.m_func);
		}
	}
	else
	{
		X::Data::Function* pFuncObj = (X::Data::Function*)pContext;
		X::AST::Func* pFunc = pFuncObj->GetFunc();
		buildtask(pFunc);
	}
	if (pFutureList)
	{
		retValue = X::Value(pFutureList);
	}
	else if(pFuture)
	{
		retValue = X::Value(pFuture);
	}
	return bOK;
}
bool U_OnEvent(X::XRuntime* rt, void* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{// on(evtName,handler)
	if (params.size() !=2)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string name = params[0].ToString();
	auto* pEvt = X::EventSystem::I().Query(name);
	if (pEvt == nullptr)
	{//Create it
		pEvt = X::EventSystem::I().Register(name);
	}
	X::Data::Function* pFuncHandler = nullptr;
	auto handler = params[1];
	if (handler.IsObject())
	{
		auto* pObjHandler = dynamic_cast<X::Data::Object*>(handler.GetObj());
		if (pObjHandler && pObjHandler->GetType() == ObjType::Function)
		{
			pFuncHandler = dynamic_cast<X::Data::Function*>(pObjHandler);
			pFuncHandler->AddRef();
		}
	}
	pEvt->Add([](void* pContext, void* pContext2, Event* pEvt) {
		if (pContext)
		{
			X::Value valEvt(pEvt);
			X::ARGS params;
			params.push_back(valEvt);
			X::KWARGS kwParams;
			X::Value retValue;
			X::Data::Function* pFuncHandler = (X::Data::Function*)pContext;
			pFuncHandler->Call((X::XRuntime*)pContext2, params, kwParams, retValue);
		}
	}, pFuncHandler,rt);

	retValue = X::Value(true);
	return true;
}
bool U_FireEvent(X::XRuntime* rt, void* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string name = params[0].ToString();
	if (kwParams.find("tid") == kwParams.end())
	{
		int tid = (int)GetThreadID();
		kwParams.emplace(std::make_pair("tid", tid));
	}
	X::EventSystem::I().Fire(name, kwParams);
	retValue = X::Value(true);
	return true;
}
bool U_AddPath(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Runtime* pRt = (X::Runtime*)rt;
	for (auto& p : params)
	{
		std::string strPath = p.ToString();
		if (!IsAbsPath(strPath))
		{
			std::string curPath = pRt->M()->GetModulePath();
			strPath = curPath + Path_Sep_S + strPath;
		}
		pRt->M()->AddSearchPath(strPath);
	}
	return true;
}
bool U_RemovePath(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Runtime* pRt = (X::Runtime*)rt;
	for (auto& p : params)
	{
		std::string strPath = p.ToString();
		if (!IsAbsPath(strPath))
		{
			std::string curPath = pRt->M()->GetModulePath();
			strPath = curPath + Path_Sep_S + strPath;
		}
		pRt->M()->RemoveSearchPath(strPath);
	}
	return true;
}
bool U_SetAttribute(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::Object* pObj = nullptr;
	std::string name;
	X::Value val;
	if (pContext == nullptr)
	{//format setattr(obj,name,value)
		if (params.size() < 3 || !params[0].IsObject())
		{
			retValue = X::Value();
			return false;
		}
		pObj = dynamic_cast<X::Data::Object*>(params[0].GetObj());
		name = params[1].ToString();
		val = params[2];
	}
	else
	{
		if (params.size() < 2)
		{
			retValue = X::Value();
			return false;
		}
		pObj = (X::Data::Object*)pContext;
		name = params[0].ToString();
		val = params[1];
	}
	auto* aBag = pObj->GetAttrBag();
	aBag->Set(name, val);
	retValue = X::Value(true);
	return true;
}
bool U_GetAttribute(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::Object* pObj = nullptr;
	std::string name;
	if (pContext == nullptr)
	{//format getattr(obj,name)
		if (params.size() < 2 || !params[0].IsObject())
		{
			retValue = X::Value();
			return false;
		}
		pObj = dynamic_cast<X::Data::Object*>(params[0].GetObj());
		name = params[1].ToString();
	}
	else
	{
		if (params.size() < 1)
		{
			retValue = X::Value();
			return false;
		}
		pObj = (X::Data::Object*)pContext;
		name = params[0].ToString();
	}
	auto* aBag = pObj->GetAttrBag();
	retValue = aBag->Get(name);
	return true;
}
bool U_DeleteAttribute(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::Object* pObj = nullptr;
	std::string name;
	if (pContext == nullptr)
	{//format delattr(obj,name)
		if (params.size() < 2 || !params[0].IsObject())
		{
			retValue = X::Value();
			return false;
		}
		pObj = dynamic_cast<X::Data::Object*>(params[0].GetObj());
		name = params[1].ToString();
	}
	else
	{
		if (params.size() < 1)
		{
			retValue = X::Value();
			return false;
		}
		pObj = (X::Data::Object*)pContext;
		name = params[0].ToString();
	}
	auto* aBag = pObj->GetAttrBag();
	aBag->Delete(name);
	retValue = X::Value(true);
	return true;
}
bool U_Each(X::XRuntime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	auto proc = [](X::XRuntime* rt, void* pContext,X::Value& keyOrIdx, X::Value& val,
		ARGS& params, KWARGS& kwParams)
	{
		X::Value vfunc = params[0];
		X::Data::Function* pFuncObj = dynamic_cast<X::Data::Function*>(vfunc.GetObj());
		X::AST::Func* pFunc = pFuncObj->GetFunc();

		X::Value retVal;
		ARGS params_to_cb;
		params_to_cb.push_back(keyOrIdx);
		params_to_cb.push_back(val);
		for (int i = 1; i < params.size(); i++)
		{
			params_to_cb.push_back(params[i]);
		}
		pFunc->Call(rt, pContext, params_to_cb, kwParams, retVal);
		return retVal;
	};
	ARGS params_proc;

	X::Data::Object* pObj = nullptr;
	if (pContext == nullptr)
	{//format delattr(obj,name)
		if (params.size() < 2 || !params[0].IsObject())
		{
			retValue = X::Value();
			return false;
		}
		pObj = dynamic_cast<X::Data::Object*>(params[0].GetObj());
		for (int i = 1; i < params.size(); i++)
		{
			params_proc.push_back(params[i]);
		}
	}
	else
	{
		if (params.size() < 1)
		{
			retValue = X::Value();
			return false;
		}
		pObj = (X::Data::Object*)pContext;
		for (int i = 0; i < params.size(); i++)
		{
			params_proc.push_back(params[i]);
		}
	}
	if (pObj)
	{
		pObj->Iterate(rt, pContext,proc, params_proc, kwParams);
	}
	return true;
}
bool Builtin::RegisterInternals()
{
	REGISTER_PACKAGE("json", X::JsonWrapper);
	REGISTER_PACKAGE("xdb", X::DevOps::DebugService);

	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (void*)U_Print, params);
	Register("load", (void*)U_Load, params);
	Register("run", (void*)U_Run, params);
	Register("runcode", (void*)U_RunCode, params);
	Register("rand", (void*)U_Rand, params);
	Register("sleep", (void*)U_Sleep, params);
	Register("time", (void*)U_Time, params);
	Register("breakpoint", (void*)U_BreakPoint, params);
	Register("taskrun", (void*)U_TaskRun, params);
	Register("threadid", (void*)U_ThreadId, params);
	Register("mainrun", (void*)U_RunInMain, params);
	Register("on", (void*)U_OnEvent, params);
	Register("fire", (void*)U_FireEvent, params);
	Register("addpath", (void*)U_AddPath, params);
	Register("removepath", (void*)U_RemovePath, params);
	Register("tostring", (void*)U_ToString, params);
	Register("bytes", (void*)U_ToBytes, params);
	Register("setattr", (void*)U_SetAttribute, params,true);
	Register("getattr", (void*)U_GetAttribute, params,true);
	Register("delattr", (void*)U_DeleteAttribute, params, true);
	Register("each", (void*)U_Each, params, true);
	return true;
}
}