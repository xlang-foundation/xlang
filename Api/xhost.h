#ifndef _X_HOST_H_
#define _X_HOST_H_
#include "value.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace X
{
	class XObj;
	class XStr;
	class XPackage;
	class XFunc;
	class XDict;
	class XBin;
	class XRuntime
	{
	public:

	};
	typedef XPackage* (*PackageCreator)(XRuntime* rt);
	typedef std::vector<X::Value> ARGS;
	typedef std::unordered_map<std::string, X::Value> KWARGS;
	typedef bool (*U_FUNC) (XRuntime* rt, void* pContext,
		std::vector<Value>& params, KWARGS& kwParams, Value& retValue);
	using EventHandler = std::function<void(const Value&)>;

	class XHost
	{
	public:
		virtual bool RegisterPackage(const char* name, PackageCreator creator) = 0;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) = 0;
		virtual XStr* CreateStr(const char* data, int size) = 0;
		virtual XDict* CreateDict() = 0;
		virtual XPackage* CreatePackage(void* pRealObj) = 0;
		virtual XFunc* CreateFunction(const char* name, U_FUNC func) = 0;
		virtual std::string StringifyString(const std::string& str) = 0;
		virtual XBin* CreateBin(char* data, size_t size) = 0;
		virtual bool ConvertToBytes(X::Value& v, X::XLStream* pStream=nullptr) = 0;
		virtual bool ConvertFromBytes(X::Value& v, X::XLStream* pStream = nullptr) = 0;
		virtual bool RunCode(std::string& moduleName, std::string& code, X::Value& retVal) = 0;
		virtual long OnEvent(const char* evtName, EventHandler handler) = 0;
		virtual void OffEvent(const char* evtName, long Cookie) = 0;
		virtual Value GetAttr(const X::Value& v, const char* attrName) = 0;
		virtual void SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal) = 0;
	};
	extern XHost* g_pXHost;
}

#endif