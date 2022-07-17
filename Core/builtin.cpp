#include "builtin.h"
#include "exp.h"
#include "object.h"
#include "funclist.h"
#include "function.h"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif#include <time.h> 
#include "utility.h"
#include "task.h"
#include <vector>
#include "Locker.h"
#include "def.h"
#include "Hosting.h"
#include <fstream>
#include <sstream>
#include "event.h"

Locker _printLock;
bool U_Print(X::Runtime* rt,void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	_printLock.Lock();
	for (auto& v : params)
	{
		std::string str = v.ToString();
		std::cout << str;
	}
	std::cout << std::endl;
	_printLock.Unlock();
	retValue = X::AST::Value(true);
	return true;
}
bool U_Load(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::AST::Value(false);
		return false;
	}
	std::string fileName = params[0].ToString();
	std::ifstream moduleFile(fileName);
	std::string code((std::istreambuf_iterator<char>(
		moduleFile)), std::istreambuf_iterator<char>());
	moduleFile.close();
	unsigned long long moduleKey = 0;
	X::Hosting::I().Load(fileName, code.c_str(), (int)code.size(), moduleKey);
	retValue = X::AST::Value(moduleKey);
	return true;
}
bool U_Run(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
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
bool U_RunInMain(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	X::Event* pEvt = X::EventSystem::I().Query("RunModule");
	if (pEvt == nullptr)
	{
		pEvt = X::EventSystem::I().Register("RunModule");
		pEvt->Add([](void* pContext, X::Event* pEvt) 
			{
				unsigned long long mKey = 0;
				auto valKey = pEvt->Get("ModuleKey");
				mKey = valKey.GetLongLong();
				X::KWARGS kwParams0;
				pEvt->CovertPropsToArgs(kwParams0);
				X::AST::Value retValue0;
				X::Hosting::I().Run(mKey, kwParams0, retValue0);
			}, rt);
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
	X::AST::Value valKey(key);
	pEvt->Set("ModuleKey", valKey);
	for (auto& it : kwParams)
	{
		pEvt->Set(it.first.c_str(), it.second);
	}
	pEvt->FireInMain();
	pEvt->Release();
	return true;
}

bool U_Rand(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	static bool init = false;
	if (!init)
	{
		srand((unsigned int)time(nullptr));
		init = true;
	}
	int r = rand();
	retValue = X::AST::Value(r);
	return true;
}
bool U_ThreadId(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	long long id = GetThreadID();
	retValue = X::AST::Value(id);
	return true;
}
bool U_Sleep(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
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
	Sleep((int)t);
	if (pContext)
	{//with a function, means after sleep, call this function
		X::Data::Function* pFuncObj = (X::Data::Function*)pContext;
		X::AST::Func* pFunc = pFuncObj->GetFunc();
		bOK = pFunc->Call(rt, nullptr,params, kwParams, retValue);
	}
	else
	{
		retValue = X::AST::Value(bOK);
	}
	return bOK;
}
bool U_Time(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::AST::Value& retValue)
{
	long long t = getCurMilliTimeStamp();
	retValue = X::AST::Value(t);
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
	std::vector<std::pair<std::string, std::string>>& params)
{
	std::string strName(name);
	AST::ExternFunc* extFunc = new AST::ExternFunc(
		strName,
		(AST::U_FUNC)func);
	m_mapFuncs.emplace(std::make_pair(name,extFunc));
	return true;
}

bool U_BreakPoint(X::Runtime* rt, void* pContext,
	X::ARGS& params,
	KWARGS& kwParams,
	X::AST::Value& retValue)
{
	rt->M()->SetDbgType(AST::dbg::Step,
		AST::dbg::Step);
	retValue = X::AST::Value(true);
	return true;
}


bool U_TaskRun(X::Runtime* rt, void* pContext,
	ARGS& params,KWARGS& kwParams,
	X::AST::Value& retValue)
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
				X::AST::Value vTask(pFuture);
				pFutureList->Add(rt, vTask);
				pFuture = nil;
			}
			X::AST::Value vTask(f);
			pFutureList->Add(rt, vTask);
		}
		else
		{
			pFuture = f;
		}
		bool bRet = tsk->Call(pFunc, rt, pContext, params, kwParams);
		return bRet;
	};
	bool bOK = true;
	auto* pContextObj = (X::Data::Object*)pContext;
	if (pContextObj->GetType() == X::Data::Type::FuncCalls)
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
		retValue = X::AST::Value(pFutureList);
	}
	else if(pFuture)
	{
		retValue = X::AST::Value(pFuture);
	}
	return bOK;
}
bool U_FireEvent(X::Runtime* rt, void* pContext,
	ARGS& params, KWARGS& kwParams,
	X::AST::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = AST::Value(false);
		return false;
	}
	std::string name = params[0].ToString();
	if (kwParams.find("tid") == kwParams.end())
	{
		int tid = (int)GetThreadID();
		kwParams.emplace(std::make_pair("tid", tid));
	}
	X::EventSystem::I().Fire(name, kwParams);
	retValue = AST::Value(true);
	return true;
}
bool Builtin::RegisterInternals()
{
	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (void*)U_Print, params);
	Register("load", (void*)U_Load, params);
	Register("run", (void*)U_Run, params);
	Register("rand", (void*)U_Rand, params);
	Register("sleep", (void*)U_Sleep, params);
	Register("time", (void*)U_Time, params);
	Register("breakpoint", (void*)U_BreakPoint, params);
	Register("taskrun", (void*)U_TaskRun, params);
	Register("threadid", (void*)U_ThreadId, params);
	Register("mainrun", (void*)U_RunInMain, params);
	Register("fire", (void*)U_FireEvent, params);
	return true;
}
}