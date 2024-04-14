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
#include "json.h"
#include "yaml.h"
#include "html.h"
#include "metascope.h"
#include "attribute.h"
#include "devops.h"
#include "msgthread.h"
#include "runtime.h"
#include "pyproxyobject.h"
#include "moduleobject.h"
#include "runtime.h"
#include "manager.h"
#include "typeobject.h"
#include "complex.h"
#include "set.h"
#include "dict.h"
#include "future.h"
#include "taskpool.h"
#include "tensor.h"
#include "tensor_cpu.h"
#include "xport.h"
#include "ast.h"
#include "time_object.h"


namespace X
{
	extern XLoad* g_pXload;
}

FORCE_INLINE static std::string CombineParamsToString(X::ARGS& params)
{
	std::string allOut;
	int p_cnt = (int)params.size();
	for (int i = 0; i < p_cnt; i++)
	{
		//todo: for linux, may need to change
		auto& v = params[i];
		if (i == 0)
		{
			allOut = v.ToString();
		}
		else
		{
			allOut += " " + v.ToString();//add space between two items
		}
	}
	return allOut;
}
Locker _printLock;
bool U_Print(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	std::string allOut = CombineParamsToString(params);
	//_printLock.Lock();
	//add new line per print
	allOut += '\n';

	X::XlangRuntime* pRT = dynamic_cast<X::XlangRuntime*>(rt);
	auto* pModule = pRT->M();
	//todo: this is a temp solution for remote call with module
	//need to consider a right soltution for runtime assign
	if (pModule == nullptr)
	{
		auto* pModuleObj = dynamic_cast<X::AST::ModuleObject*>(pContext);
		if (pModuleObj)
		{
			pModule = pModuleObj->M();
		}
	}
	bool IsRenderByPrimtive = false;
	if (pModule)
	{
		auto& outputPrimitive = pModule->GetPrimitive(X::AST::module_primitive::Output);
		if (outputPrimitive.primitive.IsObject())
		{
			X::XObj* pObj = outputPrimitive.primitive.GetObj();
			if (pObj)
			{
				X::ARGS params_p(0);
				X::KWARGS kwargs_p;
				params_p.push_back(allOut);
				IsRenderByPrimtive = pObj->Call(outputPrimitive.rt,
					nullptr, params_p, kwargs_p, retValue);
			}
		}
	}
	//_printLock.Unlock();
	if (!IsRenderByPrimtive)
	{
		#ifdef _WIN32
		SetConsoleOutputCP(CP_UTF8);
		#endif
		std::cout << allOut;
	}
	return true;
}
bool U_Input(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	std::string in;
	std::cin >> in;
	retValue = X::Value(in);
	return true;
}
bool U_Alert(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	std::string allOut = CombineParamsToString(params);
#if (WIN32)
	MessageBox(NULL, allOut.c_str(),"XLang", MB_OK);
#else
#endif
	return true;
}
bool U_Load(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
	X::Hosting::I().Load(fileName.c_str(), code.c_str(), (int)code.size(), moduleKey);
	retValue = X::Value(moduleKey);
	return true;
}
bool U_LoadS(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string code = params[0].ToString();
	unsigned long long moduleKey = 0;
	X::Hosting::I().Load("default", code.c_str(), (int)code.size(), moduleKey);
	retValue = X::Value(moduleKey);
	return true;
}

bool U_LoadModule(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if(params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string modulePath = params[0].ToString();
	std::ifstream moduleFile(modulePath);
	std::string code((std::istreambuf_iterator<char>(
		moduleFile)), std::istreambuf_iterator<char>());
	moduleFile.close();
	if (code.size() == 0)
	{
		retValue = X::Value();
		return false;
	}

	X::Value objModule;
	bool bOK = X::g_pXHost->LoadModule(modulePath.c_str(), code.c_str(),(int)code.size(), objModule);
	if (bOK)
	{
		X::Value moduleRet;
		X::g_pXHost->RunModule(objModule, moduleRet, true);
	}
	retValue = objModule;
	return bOK;
}
bool U_UnloadModule(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	bool bOK =  X::g_pXHost->UnloadModule(params[0]);
	retValue = X::Value(bOK);
	return bOK;	
}
bool U_RunByteCode(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
bool U_Run(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
		if (it != nullptr)
		{
			key = it->val.GetLongLong();
		}
	}
	return X::Hosting::I().Run(key, kwParams, retValue);
}
bool U_RunCode(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
	std::vector<X::Value> passInParams;
	return X::Hosting::I().Run(moduleName.c_str(), code.c_str(),
		(int)code.size(), passInParams,retValue);
}
bool U_RunFragmentCode(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() < 1)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string code = params[0].ToString();
	std::vector<std::string> passInParams;
	return X::Hosting::I().RunCodeLine(code.c_str(),
		(int)code.size(), retValue);
}
bool U_RunInMain(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::ObjectEvent* pEvt = X::EventSystem::I().Query("RunModule");
	if (pEvt == nullptr)
	{
		pEvt = X::EventSystem::I().Register("RunModule");
		pEvt->Add([rt](X::XRuntime* rt1, X::XObj* pContext,
			X::ARGS& params, X::KWARGS& kwParams, X::Value& retValue)
			{
				unsigned long long mKey = 0;
				auto it = kwParams.find("ModuleKey");
				if (it)
				{
					mKey = it->val.GetLongLong();
				}
				X::Hosting::I().Run(mKey, kwParams, retValue);
			});
	}
	unsigned long long key = 0;
	if (params.size() > 0)
	{
		key = params[0].GetLongLong();
		X::Value valKey(key);
		kwParams.Add("ModuleKey", valKey);
	}
	pEvt->FireInMain(rt,pContext,params,kwParams);
	pEvt->DecRef();
	return true;
}

bool U_Rand(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
bool U_ThreadId(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long id = GetThreadID();
	retValue = X::Value(id);
	return true;
}
bool U_ProcessId(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long id = GetPID();
	retValue = X::Value(id);
	return true;
}
bool U_Sleep(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
			if (it)
			{
				t = it->val.GetLongLong();
			}
		}
	}
	else
	{//must put into kwargs with time=t
		auto it = kwParams.find("time");
		if (it)
		{
			t = it->val.GetLongLong();
		}
	}
	MS_SLEEP((int)t);
	if (pContext)
	{//with a function, means after sleep, call this function
		X::Data::Function* pFuncObj = dynamic_cast<X::Data::Function*>(pContext);
		if (pFuncObj)
		{
			X::AST::Func* pFunc = pFuncObj->GetFunc();
			bOK = pFunc->Call(rt, nullptr, params, kwParams, retValue);
		}
	}
	else
	{
		retValue = X::Value(bOK);
	}
	return bOK;
}
bool U_Time(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long long t = getCurMilliTimeStamp();
	retValue = X::Value(t);
	return true;
}
bool U_NewModule(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	retValue =  X::Hosting::I().NewModule();
	return true;
}
bool U_GetModuleFromKey(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	unsigned long long key = 0;
	if (params.size() > 0)
	{
		key = params[0].GetLongLong();
		X::Value valKey(key);
		kwParams.Add("ModuleKey", valKey);
	}
	if (key == 0)
	{
		return false;
	}
	auto* pModule = X::Hosting::I().QueryModule(key);
	if (pModule == nullptr)
	{
		return false;
	}
	auto* pModuleObj = new X::AST::ModuleObject(pModule);
	retValue =  X::Value(pModuleObj);
	return true;
}
bool U_ObjectLock(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		if (pContext)
		{
			auto* pDataObj = dynamic_cast<X::Data::Object*>(pContext);
			if (pDataObj)
			{
				pDataObj->ExternLock();
			}
		}
	}
	else
	{
		auto& v = params[0];
		if (v.IsObject())
		{
			auto* pDataObj = dynamic_cast<X::Data::Object*>(v.GetObj());
			if (pDataObj)
			{
				pDataObj->ExternLock();
			}
		}
	}
	return true;
}
bool U_ObjectUnlock(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		if (pContext)
		{
			auto* pDataObj = dynamic_cast<X::Data::Object*>(pContext);
			if (pDataObj)
			{
				pDataObj->ExternUnlock();
			}
		}
	}
	else
	{
		auto& v = params[0];
		if (v.IsObject())
		{
			auto* pDataObj = dynamic_cast<X::Data::Object*>(v.GetObj());
			if (pDataObj)
			{
				pDataObj->ExternUnlock();
			}
		}
	}
	return true;
}
namespace X {
void Builtin::Cleanup()
{
	m_lock.Lock();
	for (auto it : m_Funcs)
	{
		it.funcObj->DecRef();
	}
	m_Funcs.clear();
	m_mapNameToIndex.clear();
	m_lock.Unlock();
}
Data::Function* Builtin::Find(std::string& name)
{
	AutoLock autoLock(m_lock);
	auto it = m_mapNameToIndex.find(name);
	if (it != m_mapNameToIndex.end())
	{
		return m_Funcs[it->second].funcObj;
	}
	else
	{
		return nil;
	}
}
bool Builtin::RegisterWithScope(const char* name, X::U_FUNC func,
	AST::Scope* pScope,
	std::vector<std::pair<std::string, std::string>>& params,
	const char* doc,
	bool regToMeta)
{
	std::string strName(name);
	AST::ExternFunc* extFunc = new AST::ExternFunc(strName, doc,func);
	auto* pFuncObj = new Data::Function(extFunc, true);
	pFuncObj->SetExtraScope(pScope);
	pFuncObj->IncRef();
	m_lock.Lock();
	int id = (int)m_Funcs.size();
	m_Funcs.push_back({ name ,pFuncObj });
	m_mapNameToIndex.emplace(std::make_pair(name, id));
	m_lock.Unlock();
	if (regToMeta)
	{
		SCOPE_FAST_CALL_AddOrGet0(idx,X::AST::MetaScope::I().GetMyScope(),strName, false);
		X::Value vFunc(pFuncObj);
		X::AST::MetaScope::I().GetMyScope()->Set(nullptr, nullptr, idx, vFunc);
	}
	return true;
}
bool Builtin::Register(const char* name, X::U_FUNC func,
	std::vector<std::pair<std::string, std::string>>& params,
	const char* doc,
	bool regToMeta)
{
	std::string strName(name);
	AST::ExternFunc* extFunc = new AST::ExternFunc(strName, doc,func);
	auto* pFuncObj = new Data::Function(extFunc,true);
	pFuncObj->IncRef();
	m_lock.Lock();
	int id = (int)m_Funcs.size();
	m_Funcs.push_back({ name ,pFuncObj});
	m_mapNameToIndex.emplace(std::make_pair(name, id));
	m_lock.Unlock();
	if (regToMeta)
	{
		SCOPE_FAST_CALL_AddOrGet0(idx,X::AST::MetaScope::I().GetMyScope(),strName, false);
		X::Value vFunc(pFuncObj);
		X::AST::MetaScope::I().GetMyScope()->Set(nullptr, nullptr, idx, vFunc);
	}
	return true;
}

bool U_BreakPoint(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	KWARGS& kwParams,
	X::Value& retValue)
{
	((X::XlangRuntime*)rt)->M()->SetDbgType(AST::dbg::Step,
		AST::dbg::Step);
	retValue = X::Value(true);
	return true;
}
bool U_ToString(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() != 1)
	{
		retValue = X::Value(false);
		return false;
	}
	bool bFmt = false;
	auto it = kwParams.find("format");
	if (it)
	{
		bFmt = it->val.IsTrue();
	}
	auto retStr = params[0].ToString(bFmt);
	retValue = X::Value(retStr);
	return true;
}
bool U_ToBytes(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
	X::Data::Binary* pBinOut = new X::Data::Binary(pData, size,true);
	retValue = X::Value(pBinOut);
	return true;
}
bool U_FromBytes(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value();
		return false;
	}
	auto& param = params[0];
	if(!param.IsObject() || param.GetObj()->GetType() != X::ObjType::Binary)
	{
		retValue = X::Value();
		return false;
	}
	auto* pBin = dynamic_cast<X::Data::Binary*>(param.GetObj());
	X::BlockStream stream(pBin->Data(), (STREAM_SIZE)pBin->Size(),false);
	auto* pXLangRt = (X::XlangRuntime*)rt;
	stream.ScopeSpace().SetContext(pXLangRt, pContext);
	stream >> retValue;
	return true;
}
bool U_GetType(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		return false;
	}
	X::Data::TypeObject* pTypeObj = new X::Data::TypeObject(params[0]);
	retValue = X::Value(pTypeObj);
	return true;
}
bool U_ToInt(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 1)
	{
		auto& v = params[0];
		auto t = v.GetType();
		if (t == X::ValueType::Str || 
			(t == X::ValueType::Object 
			&& v.GetObj()->GetType() == X::ObjType::Str))
		{
			std::string strVal = v.ToString();
			int nVal = 0;
			SCANF(strVal.c_str(), "%d", &nVal);
			retValue = X::Value(nVal);
		}
		else
		{
			retValue = X::Value((int)v);
		}
		return true;
	}
	else
	{
		retValue = X::Value(false);
		return false;
	}
}
bool U_ToFloat(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 1)
	{
		auto& v = params[0];
		auto t = v.GetType();
		if (t == X::ValueType::Str ||
			(t == X::ValueType::Object
				&& v.GetObj()->GetType() == X::ObjType::Str))
		{
			std::string strVal = v.ToString();
			double dVal = 0;
			SCANF(strVal.c_str(), "%lf", &dVal);
			retValue = X::Value(dVal);
		}
		else
		{
			retValue = X::Value((float)v);
		}
		return true;
	}
	else
	{
		retValue = X::Value(false);
		return false;
	}
}
bool U_TaskRun(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params,KWARGS& kwParams,
	X::Value& retValue)
{
	//if params has a TaskPool, will get it
	X::Value taskPool;
	ARGS params0(params.size());
	for (auto& p : params)
	{
		if (p.IsObject() && p.GetObj()->GetType() == ObjType::TaskPool)
		{
			taskPool = p;
		}
		else
		{
			params0.push_back(p);
		}
	}
	params0.Close();
	if (taskPool.IsInvalid())
	{
		auto it = kwParams.find("TaskPool");
		if (it)
		{
			if (it->val.IsObject())
			{
				taskPool = it->val;
			}
		}
	}
	X::Data::List* pFutureList = nil;
	X::Data::Future* pFuture = nil;
	auto buildtask = [&](X::Value& valFunc)
	{
		X::Task* tsk = new X::Task();
		tsk->SetTaskPool(taskPool);
		//tsk will be released by Future
		X::Data::Future* f = new X::Data::Future(tsk);
		X::Value varF(f);
		tsk->SetFuture(varF);
		if (pFuture || pFutureList)
		{
			if (pFutureList == nil)
			{
				pFutureList = new Data::List();
				X::Value vTask(pFuture);
				pFutureList->Add((X::XlangRuntime*)rt, vTask);
				pFuture = nil;
			}
			X::Value vTask(f);
			pFutureList->Add((X::XlangRuntime*)rt, vTask);
		}
		else
		{
			pFuture = f;
		}
		bool bRet = tsk->Call(valFunc, (X::XlangRuntime*)rt, pContext, params0, kwParams);
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
			//todo:
			//buildtask(i.m_func);
		}
	}
	else
	{
		X::Value valFunc(pContext);
		buildtask(valFunc);
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
bool U_OnEvent(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
			pFuncHandler->IncRef();
			pEvt->Add(pFuncHandler);
		}
	}

	retValue = X::Value(true);
	return true;
}
bool U_FireEvent(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	std::string name = params[0].ToString();
	ARGS newParams(params.size()-1);
	for (int i = 1; i < params.size(); i++)
	{
		newParams.push_back(params[i]);
	}
	bool inMain = false;
	if (!kwParams.find("tid"))
	{
		int tid = (int)GetThreadID();
		X::Value varTid(tid);
		kwParams.Add("tid", varTid);
	}
	auto it = kwParams.find("mainthread");
	if (it)
	{
		inMain = it->val.IsTrue();
	}
	X::EventSystem::I().Fire(rt,pContext,name, newParams,kwParams, inMain);
	retValue = X::Value(true);
	return true;
}
bool U_Await(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	ARGS& params, KWARGS& kwParams,
	X::Value& retValue)
{
	bool retVal = false;
	if (pContext == nullptr)
	{
		int timeout = -1;
		//check last parameter if it is number,as timeout
		int size = params.size();
		if (size > 0)
		{
			auto& v_last = params[size - 1];
			if (v_last.IsNumber())
			{
				timeout = (int)v_last;
				size--;
			}
			for (int i = 0; i < size; i++)
			{
				auto& v = params[i];
				if (v.IsObject())
				{
					Data::Object* pObj = dynamic_cast<Data::Object*>(v.GetObj());
					retVal = pObj->wait(timeout);
				}
			}
		}
	}
	else
	{
		int timeout = -1;
		auto* pObj = dynamic_cast<X::Data::Object*>(pContext);
		if (params.size() >= 1)
		{
			timeout = (int)params[0];
		}
		retVal = pObj->wait(timeout);
	}
	retValue = X::Value(retVal);
	return true;
}
bool U_AddPath(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
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
bool U_RemovePath(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::XlangRuntime* pRt = (X::XlangRuntime*)rt;
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
bool U_SetAttribute(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
bool U_GetAttribute(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
bool U_DeleteAttribute(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
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
bool U_Each(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	auto proc = [](X::XRuntime* rt,X::XObj* pContext,X::Value& keyOrIdx, X::Value& val,
		ARGS& params, KWARGS& kwParams)
	{
		X::Value vfunc = params[0];
		X::Data::Function* pFuncObj = dynamic_cast<X::Data::Function*>(vfunc.GetObj());
		X::AST::Func* pFunc = pFuncObj->GetFunc();

		X::Value retVal;
		ARGS params_to_cb(params.size()-1+2);
		params_to_cb.push_back(keyOrIdx);
		params_to_cb.push_back(val);
		for (int i = 1; i < params.size(); i++)
		{
			params_to_cb.push_back(params[i]);
		}
		pFunc->Call(rt, pContext, params_to_cb, kwParams, retVal);
		return retVal;
	};
	ARGS params_proc(params.size());

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
	params_proc.Close();
	if (pObj)
	{
		pObj->Iterate(rt, pContext,proc, params_proc, kwParams,retValue);
	}
	return true;
}
bool U_LRpc_Listen(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	long port = 0;
	if (params.size() > 0)
	{
		port = (long)(int)params[0];
	}
	if (port != 0)
	{
		MsgThread::I().SetPort(port);
	}
	if (params.size() > 1 && params[1].IsTrue())
	{//block mode
		MsgThread::I().run();
	}
	else
	{
		X::MsgThread::I().Start();
	}
	return true;
}
bool U_PushWritePad(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() < 1)
	{
		return false;
	}
	auto* pRuntime = dynamic_cast<XlangRuntime*>(rt);
	std::string alais;
	if (params.size() >= 2)
	{
		alais = params[1].ToString();
	}
	int padIndex = pRuntime->PushWritePad(params[0], alais);
	retValue = X::Value(padIndex);
	return true;
}
bool U_PopWritePad(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	auto* pRuntime = dynamic_cast<XlangRuntime*>(rt);
	pRuntime->PopWritePad();
	return true;
}

bool U_To_XObj(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	bool bOK = false;
	//check if it is PyObject
	auto& obj = params[0];
	if (obj.IsObject() 
		&& obj.GetObj()->GetType() == ObjType::PyProxyObject)
	{
		auto* pPyObj = dynamic_cast<Data::PyProxyObject*>(obj.GetObj());
		if (pPyObj)
		{
			bOK = pPyObj->ToValue(retValue);
		}
	}
	return true;
}

bool U_Extract_Data_ToBin(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	if (params.size() == 0)
	{
		retValue = X::Value(false);
		return false;
	}
	bool bOK = false;
	//check if it is PyObject
	auto& obj = params[0];
	if (obj.IsObject()
		&& obj.GetObj()->GetType() == ObjType::PyProxyObject)
	{
		auto* pPyObj = dynamic_cast<Data::PyProxyObject*>(obj.GetObj());
		if (pPyObj)
		{
			bOK = pPyObj->ToBin(retValue);
		}
	}
	return true;
}
bool U_GetModuleFileName(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	auto* pRuntime = dynamic_cast<XlangRuntime*>(rt);
	if (pRuntime->M())
	{
		retValue = X::Value(pRuntime->M()->GetModuleName());
	}
	return true;
}
bool U_GetArgs(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	XlangRuntime* xlRt = dynamic_cast<XlangRuntime*>(rt);
	auto& args = xlRt->M()->GetArgs();
	X::List li_args;
	for (auto& s : args)
	{
		li_args += s;
	}
	retValue = li_args;
	return true;
}

bool U_CreateBaseObject(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::Object* pBaseObj = new X::Data::Object();
	retValue = X::Value(pBaseObj);
	return true;
}

bool U_CreateComplexObject(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	double real = 0;
	double imaginary = 0;

	if (params.size() == 1) {
		real = (double)params[0];	
	}
	else if (params.size() >= 2) {
		real = (double)params[0];
		imaginary = (double)params[1];
	}
	X::Data::Complex* pComplexObj = new X::Data::Complex(real, imaginary);
	retValue = X::Value(pComplexObj);
	return true;
}

bool U_CreateSetObject(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::mSet* pSetObj;
	if (params.size() == 0) {
		pSetObj = new X::Data::mSet();
	}
	else {
		pSetObj = new X::Data::mSet(params);
	}

	retValue = X::Value(pSetObj);
	return true;
}

bool U_Event_Loop(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	EventSystem::I().Loop();
	retValue = X::Value(true);
	return true;
}
bool U_RunNewInstance(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	XlangRuntime* xlRt = dynamic_cast<XlangRuntime*>(rt);
	std::string appPath(X::g_pXload->GetConfig().appPath);
	std::string appFullName(X::g_pXload->GetConfig().appFullName);
	std::string cmd;
	if (params.size() > 0)
	{
		X::Value p1 = params[0];
#if 0
		std::string codePack;
		if (p1.IsObject() && p1.GetObj()->GetType() == ObjType::Function)
		{
			X::BlockStream stream;
			for (auto& v : params)
			{
				stream << v;
			}
			auto size = stream.Size();
			char* pData = new char[size];
			stream.FullCopyTo(pData, size);
			X::Data::Binary* pBinOut = new X::Data::Binary(pData, size, true);
			pBinOut->IncRef();
			auto str_abi = pBinOut->ToString();
			codePack = str_abi;
			X::g_pXHost->ReleaseString(str_abi);
			std::cout << codePack << std::endl;
			pBinOut->DecRef();
		}
		else
		{
			codePack = p1.ToString();
		}
		cmd = appFullName + " -c " + codePack;
#endif
		cmd = appFullName + " " + p1.ToString();
	}
	unsigned long procId = 0;
	bool bOK = RunProcess(cmd, appPath, false, procId);
	if (bOK)
	{
		retValue = X::Value((long)procId);
	}
	return bOK;
}
bool U_CreateTaskPool(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	X::Data::TaskPool* pPool = new X::Data::TaskPool();
	int num = 1;
	if (params.size() > 0)
	{
		num = (int)params[0];
	}
	else
	{
		auto it = kwParams.find("max_task_num");
		if (it)
		{
			num = (int)it->val;
		}
	}
	bool bRunInUI = false;
	auto it = kwParams.find("run_in_ui");
	if (it)
	{
		bRunInUI = (bool)it->val;
	}
	pPool->SetThreadNum(num);
	pPool->SetInUIThread(bRunInUI);
	retValue = X::Value(pPool);
	return true;
}
bool U_PythonRun(X::XRuntime* rt, X::XObj* pThis, X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	std::string code;
	if (params.size() > 0)
	{
		code = params[0].ToString();
	}
	else
	{
		retValue = X::Value(false);
		return false;
	}
	if (g_pPyHost)
	{
		retValue = g_pPyHost->Exec(code.c_str());
	}
	return true;
}
bool U_CreateDict(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	bool bOK = false;
	if (params.size() > 0)
	{
		auto& p0 = params[0];
		if (p0.IsObject() && p0.GetObj()->GetType() == ObjType::Dict)
		{
			retValue = p0;
			bOK = true;
		}
	}
	else
	{
		auto* pDict = new X::Data::Dict();
		retValue = X::Value(pDict);
		bOK = true;
	}
	return bOK;
}

bool U_CreateTensor(X::XRuntime* rt,X::XObj* pThis,X::XObj* pContext,
	X::ARGS& params,
	X::KWARGS& kwParams,
	X::Value& retValue)
{
	bool bOK = true;
	auto* pTensor = new X::Data::Tensor();

	std::string name;
	auto it = kwParams.find(Tensor_Name);
	if (it)
	{
		name = it->val.ToString();
	}
	pTensor->SetName(name);
	//if there is init data, will skip shape
	X::Value initData;
	if (params.size() > 0)
	{
		initData = params[0];
	}

	int dtype = (int)X::TensorDataType::FLOAT;
	it = kwParams.find(Tensor_DType);
	if (it)
	{
		dtype = (int)it->val;
	}
	else if(initData.IsValid())
	{
		if (!initData.IsList())
		{
			auto ty = initData.GetType();
			switch (ty)
			{
			case X::ValueType::Int64:
				dtype = (int)TensorDataType::LONGLONG;
				break;
			case X::ValueType::Double:
				dtype = (int)TensorDataType::DOUBLE;
				break;
			default:
				break;
			}
		}
	}
	pTensor->SetDataType((X::TensorDataType)dtype);
	Port::vector<int> shapes(0);
	it = kwParams.find(Tensor_Shape);
	if (it)
	{
		auto val = it->val;
		if (val.IsObject())
		{
			if (val.GetObj()->GetType() == ObjType::List)
			{
				List valList(val);
				auto size = valList.Size();
				shapes.resize(size);
				for (long long i = 0; i < size; i++)
				{
					shapes.push_back((int)valList->Get(i));
				}
			}
		}
		pTensor->SetShape(shapes);
	}
	bOK = pTensor->Create(initData);
	if (bOK)
	{
		retValue = X::Value(pTensor);
	}
	else
	{
		delete pTensor;
	}
	return bOK;
}
bool Builtin::RegisterInternals()
{
	XPackage* pBuiltinPack = dynamic_cast<XPackage*>(this);
	X::Value valBuiltinPack(pBuiltinPack);
	X::Manager::I().Register("builtin", valBuiltinPack);
	X::RegisterPackage<X::JsonWrapper>(m_libName.c_str(), "json");
	X::RegisterPackage<X::AST::AstWrapper>(m_libName.c_str(),"ast");
	X::RegisterPackage<X::YamlWrapper>(m_libName.c_str(),"yaml");
	X::RegisterPackage<X::HtmlWrapper>(m_libName.c_str(), "html");
	X::RegisterPackage<X::DevOps::DebugService>(m_libName.c_str(),"xdb");
	X::RegisterPackage<X::CpuTensor>(m_libName.c_str(),"CpuTensor");
	X::RegisterPackage<X::TimeObject>(m_libName.c_str(), "time");

	std::vector<std::pair<std::string, std::string>> params;
	Register("print", (X::U_FUNC)U_Print, params,"print(...)");
	Register("input", (X::U_FUNC)U_Input, params,"[var = ]input()");
	Register("alert", (X::U_FUNC)U_Alert, params, "alert(...)");
	Register("load", (X::U_FUNC)U_Load, params,"moodule = load(filename)");
	Register("loads", (X::U_FUNC)U_LoadS, params, "moodule = loads(code)");
	Register("loadModule", (X::U_FUNC)U_LoadModule, params, "moduleObj = loadModule(filename)");
	Register("unloadModule", (X::U_FUNC)U_UnloadModule, params, "unloadModule(moduleObj)");
	Register("run", (X::U_FUNC)U_Run, params,"run(module:loaded by call load func)");
	Register("runcode", (X::U_FUNC)U_RunCode, params,"runcode(moduleName,code)");
	Register("runfragmentcode", (X::U_FUNC)U_RunFragmentCode, params, "runfragmentcode(code)");
	Register("runbytecode", (X::U_FUNC)U_RunByteCode, params,"runbytecode(code_in_bytes)");
	Register("rand", (X::U_FUNC)U_Rand, params,"rand() return an integer");
	Register("sleep", (X::U_FUNC)U_Sleep, params,
		"sleep(milliseconds|time=milliseconds) or a_func.sleep(time=milliseconds), \
		after the sleep will call this function");
	Register("time", (X::U_FUNC)U_Time, params);
	Register("breakpoint", (X::U_FUNC)U_BreakPoint, params);
	Register("pushWritepad", (X::U_FUNC)U_PushWritePad, params,"pushWritepad(obj which has WritePad(input) func)");
	Register("popWritepad", (X::U_FUNC)U_PopWritePad, params,"popWritepad() pop up last WritePad");
	Register("taskrun", (X::U_FUNC)U_TaskRun, params,"",true);
	Register("threadid", (X::U_FUNC)U_ThreadId, params);
	Register("pid", (X::U_FUNC)U_ProcessId, params);
	Register("mainrun", (X::U_FUNC)U_RunInMain, params);
	Register("on", (X::U_FUNC)U_OnEvent, params);
	Register("fire", (X::U_FUNC)U_FireEvent, params);
	Register("wait", (X::U_FUNC)U_Await, params,"wait on an event or task",true);
	Register("addpath", (X::U_FUNC)U_AddPath, params);
	Register("removepath", (X::U_FUNC)U_RemovePath, params);
	Register("tostring", (X::U_FUNC)U_ToString, params);
	Register("bytes", (X::U_FUNC)U_ToBytes, params);
	Register("fromBytes", (X::U_FUNC)U_FromBytes, params);
	Register("setattr", (X::U_FUNC)U_SetAttribute, params, "", true);
	Register("getattr", (X::U_FUNC)U_GetAttribute, params, "", true);
	Register("lock", (X::U_FUNC)U_ObjectLock, params, "", true);
	Register("unlock", (X::U_FUNC)U_ObjectUnlock, params, "", true);
	Register("delattr", (X::U_FUNC)U_DeleteAttribute, params, "", true);
	Register("each", (X::U_FUNC)U_Each, params, "", true);
	Register("lrpc_listen", (X::U_FUNC)U_LRpc_Listen, params, "", true);
	Register("to_xlang", (X::U_FUNC)U_To_XObj, params, "to_xlang", true);
	Register("to_bin", (X::U_FUNC)U_Extract_Data_ToBin, params, "to_bin", true);
	Register("get_args", (X::U_FUNC)U_GetArgs, params);
	Register("get_module_filename", (X::U_FUNC)U_GetModuleFileName, params);
	Register("new_module", (X::U_FUNC)U_NewModule, params);
	Register("get_modulebykey", (X::U_FUNC)U_GetModuleFromKey, params);
	Register("run_new_instance", (X::U_FUNC)U_RunNewInstance, params);
	Register("int", (X::U_FUNC)U_ToInt, params);
	Register("float", (X::U_FUNC)U_ToFloat, params);
	Register("str", (X::U_FUNC)U_ToString, params);
	Register("type", (X::U_FUNC)U_GetType, params);
	Register("object", (X::U_FUNC)U_CreateBaseObject, params);
	Register("event_loop", (X::U_FUNC)U_Event_Loop, params);
	Register("complex", (X::U_FUNC)U_CreateComplexObject, params);
	Register("set", (X::U_FUNC)U_CreateSetObject, params);
	Register("taskpool", (X::U_FUNC)U_CreateTaskPool, params,"taskpool(max_task_num=num,run_in_ui=true|false) or taskpool(task_num)");
	Register("dict", (X::U_FUNC)U_CreateDict, params,"d = dict()|dict({key:value...})");
	Register("pyrun", (X::U_FUNC)U_PythonRun, params, "pyrun(code)");
	RegisterWithScope("tensor", (X::U_FUNC)U_CreateTensor,X::Data::Tensor::GetBaseScope(),params, "t = tensor()|tensor(init values)");
	return true;
}

int Builtin::AddMember(PackageMemberType type, const char* name, const char* doc, bool keepRawParams)
{
	return 0;
}
int Builtin::QueryMethod(const char* name, bool* pKeepRawParams)
{
	AutoLock autoLock(m_lock);
	auto it = m_mapNameToIndex.find(name);
	if (it != m_mapNameToIndex.end())
	{
		return it->second;
	}
	else
	{
		return -1;
	}
}
void* Builtin::GetEmbedObj()
{
	return nullptr;
}
bool Builtin::Init(int varNum)
{
	return false;
}
bool Builtin::SetIndexValue(int idx, Value& v)
{
	return false;
}
bool Builtin::GetIndexValue(int idx, Value& v)
{
	AutoLock autoLock(m_lock);
	v = X::Value(m_Funcs[idx].funcObj);
	return true;
}
void Builtin::RemoveALl()
{
}
}