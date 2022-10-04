#ifndef _X_HOST_H_
#define _X_HOST_H_
#include "value.h"
#include "xload.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace X
{
	class XObj;
	class XStr;
	class XPackage;
	class XFunc;
	class XProp;
	class XEvent;
	class XDict;
	class XBin;
	class XRemoteObject;
	class XProxy;
	class XRuntime
	{
	public:
		virtual bool CreateEmptyModule() = 0;
	};
	typedef XPackage* (*PackageCreator)();
	typedef XProxy* (*XProxyCreator)(std::string& url);
	typedef std::vector<X::Value> ARGS;
	typedef std::unordered_map<std::string, X::Value> KWARGS;
	//todo: evaluate performace change to use std::function
	typedef bool (*U_FUNC_bak) (XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwParams, Value& retValue);
	using U_FUNC = std::function<bool(XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwParams, Value& retValue)>;
	using U_FUNC_EX = std::function<bool(XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwParams, X::Value& trailer,Value& retValue)>;
	using EventHandler = std::function<void(XRuntime* rt, XObj* pContext,
		ARGS& params,KWARGS& kwParams, Value& retValue)>;
	using OnEventHandlerChanged = std::function<void(bool AddOrRemove,int handlerCnt)>;
	class XHost
	{
	public:
		virtual XRuntime* CreateRuntime() = 0;
		virtual XRuntime* GetCurrentRuntime() = 0;
		virtual bool RegisterPackage(const char* name, PackageCreator creator) = 0;
		virtual bool RegisterPackage(const char* name, Value& objPackage) = 0;
		virtual XObj* QueryMember(XRuntime* rt, XObj* pObj, const char* name) = 0;
		virtual bool QueryPackage(XRuntime* rt,const char* name, Value& objPackage) = 0;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) = 0;
		virtual XStr* CreateStr(const char* data, int size) = 0;
		virtual XDict* CreateDict() = 0;
		virtual XPackage* CreatePackage(void* pRealObj) = 0;
		virtual XEvent* CreateXEvent(const char* name) = 0;
		virtual XFunc* CreateFunction(const char* name, U_FUNC func,X::XObj* pContext=nullptr) = 0;
		virtual XFunc* CreateFunctionEx(const char* name, U_FUNC_EX func, X::XObj* pContext = nullptr) = 0;
		virtual XProp* CreateProp(const char* name, U_FUNC setter, U_FUNC getter) = 0;
		virtual std::string StringifyString(const std::string& str) = 0;
		virtual XBin* CreateBin(char* data, size_t size) = 0;
		virtual X::XLStream* CreateStream(const char* buf=nullptr,long long size=0) = 0;
		virtual void ReleaseStream(X::XLStream* pStream) = 0;
		virtual XRemoteObject* CreateRemoteObject(XProxy* proxy) = 0;
		virtual bool ConvertToBytes(X::Value& v, X::XLStream* pStream=nullptr) = 0;
		virtual bool ToBytes(X::Value& input, X::Value& output) = 0;
		virtual bool FromBytes(X::Value& input, X::Value& output) = 0;
		virtual bool ConvertFromBytes(X::Value& v, X::XLStream* pStream = nullptr) = 0;
		virtual bool RunCode(std::string& moduleName, std::string& code, X::Value& retVal) = 0;
		virtual long OnEvent(const char* evtName, EventHandler handler) = 0;
		virtual void OffEvent(const char* evtName, long Cookie) = 0;
		virtual Value GetAttr(const X::Value& v, const char* attrName) = 0;
		virtual void SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal) = 0;
		virtual AppEventCode HandleAppEvent(int signum) = 0;
		virtual bool Lrpc_Listen(int port,bool blockMode = false) = 0;
		virtual bool Import(XRuntime* rt,const char* moduleName, 
					const char* from, const char* thru, X::Value& objPackage) = 0;
	};
	extern XHost* g_pXHost;
}

#endif