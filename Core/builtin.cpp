#include "builtin.h"
#include "exp.h"
#include <iostream>
#include <time.h> 
#include "utility.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <vector>

bool U_Print(std::vector<X::AST::Value>& params,
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
bool U_Rand(std::vector<X::AST::Value>& params,
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
bool U_Sleep(std::vector<X::AST::Value>& params,
	X::AST::Value& retValue)
{
	if (params.size()>0)
	{
		long long t = params[0].GetLongLong();
		Sleep(t);
	}
	retValue = X::AST::Value(true);
	return true;
}
bool U_Time(std::vector<X::AST::Value>& params,
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
bool Builtin::RegisterInternals()
{
	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (void*)U_Print, params);
	Register("rand", (void*)U_Rand, params);
	Register("sleep", (void*)U_Sleep, params);
	Register("time", (void*)U_Time, params);
	return true;
}
}