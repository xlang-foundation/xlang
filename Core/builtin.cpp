#include "builtin.h"
#include "exp.h"
#include "object.h"
#include <iostream>
#include <time.h> 
#include "utility.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <vector>

bool U_Print(X::AST::Module* pModule,void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
	X::AST::Value& retValue)
{
	for (auto& v : params)
	{
		std::string str = v.ToString();
		std::cout << str;
	}
	std::cout << std::endl;
	retValue = X::AST::Value(true);
	return true;
}
bool U_Rand(X::AST::Module* pModule, void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
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
bool U_Sleep(X::AST::Module* pModule, void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
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
	Sleep(t);
	if (pContext)
	{//with a function, means after sleep, call this function
		X::Data::Function* pFuncObj = (X::Data::Function*)pContext;
		X::AST::Func* pFunc = pFuncObj->GetFunc();
		bOK = pFunc->Call(pModule, nullptr, params, kwParams, retValue);
	}
	else
	{
		retValue = X::AST::Value(bOK);
	}
	return bOK;
}
bool U_Time(X::AST::Module* pModule, void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
	X::AST::Value& retValue)
{
	long long t = getCurMilliTimeStamp();
	retValue = X::AST::Value(t);
	return true;
}


namespace X {
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

bool U_BreakPoint(X::AST::Module* pModule, void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
	X::AST::Value& retValue)
{
	pModule->SetDbg(AST::dbg::Step);
	retValue = X::AST::Value(true);
	return true;
}
bool U_TaskRun(X::AST::Module* pModule, void* pContext,
	std::vector<X::AST::Value>& params,
	std::unordered_map<std::string, X::AST::Value>& kwParams,
	X::AST::Value& retValue)
{
	X::Data::Function* pFuncObj = (X::Data::Function*)pContext;
	X::AST::Func* pFunc = pFuncObj->GetFunc();
	bool bOK =  pFunc->Call(pModule, nullptr, params, kwParams, retValue);
	return bOK;
}
bool Builtin::RegisterInternals()
{
	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (void*)U_Print, params);
	Register("rand", (void*)U_Rand, params);
	Register("sleep", (void*)U_Sleep, params);
	Register("time", (void*)U_Time, params);
	Register("breakpoint", (void*)U_BreakPoint, params);
	Register("taskrun", (void*)U_TaskRun, params);
	return true;
}
}