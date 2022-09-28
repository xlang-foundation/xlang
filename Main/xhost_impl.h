#pragma once
#include "xhost.h"

namespace X
{
	class XHost_Impl :
		public XHost
	{
	public:
		virtual XRuntime* CreateRuntime() override;
		virtual XRuntime* GetCurrentRuntime() override;
		virtual XStr* CreateStr(const char* data, int size) override;
		virtual bool RegisterPackage(const char* name, PackageCreator creator) override;
		virtual bool RegisterPackage(const char* name, Value& objPackage) override;
		virtual XObj* QueryMember(XRuntime* rt, XObj* pObj, const char* name) override;
		virtual bool QueryPackage(XRuntime* rt, const char* name, Value& objPackage) override;
		virtual XPackage* CreatePackage(void* pRealObj) override;
		virtual XEvent* CreateXEvent(const char* name) override;
		virtual XFunc* CreateFunction(const char* name, U_FUNC func, X::XObj* pContext = nullptr) override;
		virtual XFunc* CreateFunctionEx(const char* name, U_FUNC_EX func, X::XObj* pContext = nullptr) override;
		virtual XProp* CreateProp(const char* name, U_FUNC setter, U_FUNC getter) override;
		virtual XDict* CreateDict() override;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) override;
		virtual std::string StringifyString(const std::string& str) override;
		virtual XBin* CreateBin(char* data, size_t size) override;
		virtual X::XLStream* CreateStream(const char* buf=nullptr, long long size=0) override;
		virtual void ReleaseStream(X::XLStream* pStream) override;
		virtual XRemoteObject* CreateRemoteObject(XProxy* proxy) override;
		virtual bool ToBytes(X::Value& input, X::Value& output) override;
		virtual bool FromBytes(X::Value& input, X::Value& output) override;
		virtual bool ConvertToBytes(X::Value& v, X::XLStream* pStream = nullptr) override;
		virtual bool ConvertFromBytes(X::Value& v, X::XLStream* pStream = nullptr) override;
		virtual bool RunCode(std::string& moduleName, std::string& code, X::Value& retVal) override;
		virtual long OnEvent(const char* evtName, EventHandler handler) override;
		virtual void OffEvent(const char* evtName, long Cookie) override;
		virtual Value GetAttr(const X::Value& v, const char* attrName) override;
		virtual void SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal) override;
		virtual AppEventCode HandleAppEvent(int signum) override;
		virtual bool Lrpc_Listen(int port, bool blockMode) override;
		virtual bool Import(XRuntime* rt, const char* moduleName,
			const char* from, const char* thru, X::Value& objPackage) override;
	};
	X::XHost* CreatXHost();
	void DestoryXHost();
}