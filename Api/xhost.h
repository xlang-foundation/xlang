#ifndef _X_HOST_H_
#define _X_HOST_H_
#include "value.h"
#include "xload.h"
#include "xport.h"


namespace X
{
	class XObj;
	class XStr;
	class XPackage;
	class XFunc;
	class XProp;
	class XEvent;
	class XDict;
	class XList;
	class XTensor;
	class XTensorOperator;
	class XTensorGraph;
	class XSet;
	class XStruct;
	class XComplex;
	class XBin;
	class XRemoteObject;
	class XProxy;
	class XCustomScope;
	class XRuntime;
	typedef XPackage* (*PackageCreator)();
	typedef long long (*PackageGetContentSizeFunc)(void* pContextObj);
	typedef bool (*PackageToBytesFunc)(void* pContextObj, X::XLStream* pStream);
	typedef bool (*PackageFromBytesFunc)(void* pContextObj, X::XLStream* pStream);
	typedef void (*PackageCleanup)(void* pContextObj);
	typedef bool (*PackageWaitFunc)(void* pContextObj,int timeout);
	typedef X::Value (*PackageInstanceIdentity)(void* pContextObj);
	typedef void* (*PackageGetEmbededParentObject)(void* pContextObj);


	using PackageAccessor = X::Port::Function <X::Value(X::XRuntime* rt, 
		X::XObj* pContext,X::Port::vector<X::Value>& IdxAry)>;
	typedef XProxy* (*XProxyCreator)(const char* url);
	typedef bool (*XProxyFilter)(const char* url);
	typedef X::Port::vector<X::Value> ARGS;
	typedef X::Port::StringMap<X::Value> KWARGS;
	
	//todo: evaluate performace change to use std::function
	typedef bool (*U_FUNC_bak) (XRuntime* rt, XObj* pContext,
		ARGS& params, KWARGS& kwParams, Value& retValue);
	typedef void(*CLEANUP)();
	using U_FUNC = X::Port::Function<bool(XRuntime* rt, XObj* pThis,XObj* pContext,
		ARGS& params, KWARGS& kwParams, Value& retValue)>;
	using U_FUNC_EX = X::Port::Function<bool(XRuntime* rt, XObj* pThis,XObj* pContext,
		ARGS& params, KWARGS& kwParams, X::Value& trailer,Value& retValue)>;
	using EventHandler = X::Port::Function<void(XRuntime* rt, XObj* pContext,
		ARGS& params,KWARGS& kwParams, Value& retValue)>;
	using OnEventHandlerChanged = X::Port::Function<void(bool AddOrRemove,int handlerCnt)>;
	using Tensor_OperatorHandler = X::Port::Function<void(X::ARGS& inputs, X::Value& retVal)>;

	typedef bool (*UI_THREAD_RUN_HANDLER) (X::Value& callable,void* pContext);

	class XHost
	{
	public:
		virtual void AddSysCleanupFunc(CLEANUP f) = 0;
		virtual XRuntime* CreateRuntime(bool bAddTopModule = false) = 0;
		virtual XRuntime* GetCurrentRuntime() = 0;
		virtual bool RegisterPackage(const char* name,PackageCreator creator) = 0;
		virtual bool RegisterPackage(const char* name,Value& objPackage) = 0;
		virtual Value QueryMember(XRuntime* rt, XObj* pObj, const char* name) = 0;
		virtual bool QueryPackage(XRuntime* rt,const char* name, Value& objPackage) = 0;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) = 0;
		virtual XStr* CreateStr(const char* data, int size) = 0;
		virtual XDict* CreateDict() = 0;
		virtual XList* CreateList() = 0;
		virtual XTensor* CreateTensor() = 0;
		virtual XStruct* CreateStruct(char* data,int size, bool asRef) = 0;
		virtual XTensorOperator* CreateTensorOperator(Tensor_OperatorHandler op, bool isUnaryOp) = 0;
		virtual XTensorGraph* CreateTensorGraph() = 0;
		virtual XSet* CreateSet() = 0;
		virtual XComplex* CreateComplex() = 0;
		virtual Value CreatePackageWithUri(const char* packageUri, X::XLStream* pStream) = 0;
		virtual XPackage* CreatePackage(void* pRealObj) = 0;
		virtual XPackage* CreatePackageProxy(XPackage* pPackage,void* pRealObj) = 0;
		virtual XEvent* CreateXEvent(const char* name) = 0;
		virtual XFunc* CreateFunction(const char* name, U_FUNC& func,X::XObj* pContext=nullptr) = 0;
		virtual XFunc* CreateFunctionEx(const char* name, U_FUNC_EX& func, X::XObj* pContext = nullptr) = 0;
		virtual XProp* CreateProp(const char* name, U_FUNC& setter, U_FUNC& getter,XObj* pContext) = 0;
		virtual const char* StringifyString(const char* str) = 0;
		virtual void ReleaseString(const char* str) = 0;
		virtual XBin* CreateBin(char* data, size_t size,bool bOwnData) = 0;
		virtual X::XLStream* CreateStream(const char* buf=nullptr,long long size=0) = 0;
		virtual X::XLStream* CreateStreamWithSameScopeSpace(X::XLStream* pRefStream,const char* buf = nullptr, long long size = 0) = 0;
		virtual void ReleaseStream(X::XLStream* pStream) = 0;
		virtual XRemoteObject* CreateRemoteObject(XProxy* proxy) = 0;
		virtual bool ConvertToBytes(X::Value& v, X::XLStream* pStream=nullptr) = 0;
		virtual bool ToBytes(X::Value& input, X::Value& output) = 0;
		virtual bool FromBytes(X::Value& input, X::Value& output) = 0;
		virtual bool ConvertFromBytes(X::Value& v, X::XLStream* pStream = nullptr) = 0;
		virtual bool WriteToStream(char* data, long long size, X::XLStream* pStream) = 0;
		virtual bool ReadFromStream(char* buffer, long long size, X::XLStream* pStream) = 0;
		virtual bool RunCode(const char* moduleName,const char* code, int codeSize,X::Value& retVal) = 0;
		virtual bool RunModuleInThread(const char* moduleName, const char* code, int codeSize,X::ARGS& args,X::KWARGS& kwargs) = 0;
		virtual bool RunCodeLine(const char* codeLine, int codeSize,X::Value& retVal) = 0;
		virtual const char* GetInteractiveCode() = 0;
		virtual long OnEvent(const char* evtName, EventHandler handler) = 0;
		virtual void OffEvent(const char* evtName, long Cookie) = 0;
		virtual Value GetAttr(const X::Value& v, const char* attrName) = 0;
		virtual void SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal) = 0;
		virtual AppEventCode HandleAppEvent(int signum) = 0;
		virtual bool Lrpc_Listen(int port,bool blockMode = false) = 0;
		virtual bool Import(XRuntime* rt,const char* moduleName, 
					const char* from, const char* thru, X::Value& objPackage) = 0;
		virtual bool CreateScopeWrapper(XCustomScope* pScope) = 0;
		virtual bool DeleteScopeWrapper(XCustomScope* pScope) = 0;
		virtual bool SetExpressionScope(XCustomScope* pScope, X::Value& expr) = 0;
		virtual bool RunExpression(X::Value& expr, X::Value& result) = 0;
		virtual bool ExtractNativeObjectFromRemoteObject(X::Value& remoteObj, X::Value& nativeObj) = 0;
		virtual void RegisterUIThreadRunHandler(UI_THREAD_RUN_HANDLER handler,void* pContext) = 0;
		virtual UI_THREAD_RUN_HANDLER GetUIThreadRunHandler() = 0;
		virtual void* GetUIThreadRunContext() =0;
		virtual X::Value CreateNdarray(int nd, unsigned long long* dims, int itemDataType, void* data) = 0;
	};
	extern XHost* g_pXHost;
}

#endif