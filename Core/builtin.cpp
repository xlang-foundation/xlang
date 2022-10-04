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
#include "msgthread.h"

Locker _printLock;
bool U_Print(X::XRuntime* rt,X::XObj* pContext,
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
bool U_Input(X::XRuntime* rt, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	std::string in;
	std::cin >> in;
	retValue = X::Value(in);
	return true;
}
bool U_Load(X::XRuntime* rt, X::XObj* pContext,
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
bool U_RunByteCode(X::XRuntime* rt, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	auto valCode = params[0];
	if (valCode.IsObject() && valCode.GetObj()->GetType() == X::ObjType::Binary)
	{
		auto* pBin = dynamic_cast<X::Data::Binary*>(valCode.GetObj());
		X::BlockStream stream(pBin->Data(), pBin->Size(), false);
		X::Value valCallable;
		valCallable.FromBytes(&stream);
		std::cout << "U_RunByteCode" << std::endl;
	}
	return true;
}
bool U_Run(X::XRuntime* rt, X::XObj* pContext,
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
bool U_RunCode(X::XRuntime* rt, X::XObj* pContext,
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

bool U_RunInMain(X::XRuntime* rt, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Event* pEvt = X::EventSystem::I().Query("RunModule");
	if (pEvt == nullptr)
	{
		pEvt = X::EventSystem::I().Register("RunModule");
		pEvt->Add([rt](X::XRuntime* rt, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
			{
				unsigned long long mKey = 0;
				auto valKey = kwParams["ModuleKey"];
				mKey = valKey.GetLongLong();
				X::Hosting::I().Run(mKey, kwParams, retValue);
			});
	}
	unsigned long long key = 0;
	if (params.size() > 0)
	{
		key = params[0].GetLongLong();
		X::Value valKey(key);
		kwParams.emplace(std::make_pair("ModuleKey", valKey));
	}
	pEvt->FireInMain(rt,pContext,params,kwParams);
	pEvt->Release();
	return true;
}

bool U_Rand(X::XRuntime* rt, X::XObj* pContext,
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
bool U_ThreadId(X::XRuntime* rt, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long id = GetThreadID();
	retValue = X::Value(id);
	return true;
}
bool U_Sleep(X::XRuntime* rt, X::XObj* pContext,
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
		X::Data::Function* pFuncObj = dynamic_cast<X::Data::Function*>(pContext);
		X::AST::Func* pFunc = pFuncObj->GetFunc();
		bOK = pFunc->Call(rt, nullptr,params, kwParams, retValue);
	}
	else
	{
		retValue = X::Value(bOK);
	}
	return bOK;
}
bool U_Time(X::XRuntime* rt, X::XObj* pContext,
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
bool Builtin::Register(const char* name, X::U_FUNC func,
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

bool U_BreakPoint(X::XRuntime* rt, XObj* pContext,
	X::ARGS& params,
	KWARGS& kwParams,
	X::Value& retValue)
{
	((X::Runtime*)rt)->M()->SetDbgType(AST::dbg::Step,
		AST::dbg::Step);
	retValue = X::Value(true);
	return true;
}
bool U_ToString(X::XRuntime* rt, XObj* pContext,
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
bool U_ToBytes(X::XRuntime* rt, XObj* pContext,
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

bool U_TaskRun(X::XRuntime* rt, XObj* pContext,
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
	auto* pContextObj = dynamic_cast<X::Data::Object*>(pContext);
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
		X::Data::Function* pFuncObj = dynamic_cast<X::Data::Function*>(pContext);
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
bool U_OnEvent(X::XRuntime* rt, XObj* pContext,
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
	auto handler = params[1];
	if (handler.IsObject())
	{
		auto* pObjHandler = dynamic_cast<X::Data::Object*>(handler.GetObj());
		if (pObjHandler && pObjHandler->GetType() == ObjType::Function)
		{
			auto* pFuncHandler = dynamic_cast<X::Data::Function*>(pObjHandler);
			pFuncHandler->AddRef();
			pEvt->Add(pFuncHandler);
		}
	}

	retValue = X::Value(true);
	return true;
}
bool U_FireEvent(X::XRuntime* rt, XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string name = params[0].ToString();
	params.erase(params.begin());
	if (kwParams.find("tid") == kwParams.end())
	{
		int tid = (int)GetThreadID();
		kwParams.emplace(std::make_pair("tid", tid));
	}
	X::EventSystem::I().Fire(rt,pContext,name, params,kwParams);
	retValue = X::Value(true);
	return true;
}
bool U_AddPath(X::XRuntime* rt, XObj* pContext,
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
bool U_RemovePath(X::XRuntime* rt, XObj* pContext,
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
bool U_SetAttribute(X::XRuntime* rt, XObj* pContext,
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
		pObj = dynamic_cast<X::Data::Object*>(pContext);
		name = params[0].ToString();
		val = params[1];
	}
	auto* aBag = pObj->GetAttrBag();
	aBag->Set(name, val);
	retValue = X::Value(true);
	return true;
}
bool U_GetAttribute(X::XRuntime* rt, XObj* pContext,
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
		pObj = dynamic_cast<X::Data::Object*>(pContext);
		name = params[0].ToString();
	}
	auto* aBag = pObj->GetAttrBag();
	retValue = aBag->Get(name);
	return true;
}
bool U_DeleteAttribute(X::XRuntime* rt, XObj* pContext,
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
		pObj = dynamic_cast<X::Data::Object*>(pContext);
		name = params[0].ToString();
	}
	auto* aBag = pObj->GetAttrBag();
	aBag->Delete(name);
	retValue = X::Value(true);
	return true;
}
bool U_Each(X::XRuntime* rt, XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	auto proc = [](X::XRuntime* rt, XObj* pContext,X::Value& keyOrIdx, X::Value& val,
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
		pObj = dynamic_cast<X::Data::Object*>(pContext);
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
bool U_LRpc_Listen(X::XRuntime* rt, XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::MsgThread::I().Start();
	return true;
}
bool Builtin::RegisterInternals()
{
	X::RegisterPackage<X::JsonWrapper>("json");
	X::RegisterPackage<X::DevOps::DebugService>("xdb");

	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (X::U_FUNC)U_Print, params);
	Register("input", (X::U_FUNC)U_Input, params);
	Register("load", (X::U_FUNC)U_Load, params);
	Register("run", (X::U_FUNC)U_Run, params);
	Register("runcode", (X::U_FUNC)U_RunCode, params);
	Register("runbytecode", (X::U_FUNC)U_RunByteCode, params);
	Register("rand", (X::U_FUNC)U_Rand, params);
	Register("sleep", (X::U_FUNC)U_Sleep, params);
	Register("time", (X::U_FUNC)U_Time, params);
	Register("breakpoint", (X::U_FUNC)U_BreakPoint, params);
	Register("taskrun", (X::U_FUNC)U_TaskRun, params, true);
	Register("threadid", (X::U_FUNC)U_ThreadId, params);
	Register("mainrun", (X::U_FUNC)U_RunInMain, params);
	Register("on", (X::U_FUNC)U_OnEvent, params);
	Register("fire", (X::U_FUNC)U_FireEvent, params);
	Register("addpath", (X::U_FUNC)U_AddPath, params);
	Register("removepath", (X::U_FUNC)U_RemovePath, params);
	Register("tostring", (X::U_FUNC)U_ToString, params);
	Register("bytes", (X::U_FUNC)U_ToBytes, params);
	Register("setattr", (X::U_FUNC)U_SetAttribute, params,true);
	Register("getattr", (X::U_FUNC)U_GetAttribute, params,true);
	Register("delattr", (X::U_FUNC)U_DeleteAttribute, params, true);
	Register("each", (X::U_FUNC)U_Each, params, true);
	Register("lrpc_listen", (X::U_FUNC)U_LRpc_Listen, params, true);
	return true;
}
}