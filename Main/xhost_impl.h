﻿/*
Copyright (C) 2024 The XLang Foundation
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#pragma once
#include "xhost.h"
#include <unordered_map>

namespace X
{
	class XHost_Impl :
		public XHost
	{
		UI_THREAD_RUN_HANDLER m_uiThreadRunHandler = nullptr;
		void* m_uiThreadRunContext = nullptr;
		std::unordered_map<std::string, X::Value> m_KV;
	public:
		virtual void AddSysCleanupFunc(CLEANUP f) override;
		virtual XRuntime* CreateRuntime(bool bAddTopModule = false) override;
		virtual XRuntime* GetCurrentRuntime() override;
		virtual XStr* CreateStr(const char* data, int size) override;
		virtual XError* CreateError(int code, const char* info) override;
		virtual bool RegisterPackage(const char* name,PackageCreator creator,void* pContext) override;
		virtual bool RegisterPackage(const char* name,Value& objPackage) override;
		virtual Value QueryMember(XRuntime* rt, XObj* pObj, const char* name) override;
		virtual bool QueryPackage(XRuntime* rt, const char* name, Value& objPackage) override;
		virtual Value CreatePackageWithUri(const char* packageUri,X::XLStream* pStream) override;
		virtual XPackage* CreatePackage(void* pRealObj) override;
		virtual XPackage* CreatePackageProxy(XPackage* pPackage, void* pRealObj) override;
		virtual XEvent* CreateXEvent(const char* name) override;
		virtual XFunc* CreateFunction(const char* name, U_FUNC& func, X::XObj* pContext = nullptr) override;
		virtual XFunc* CreateFunctionEx(const char* name, U_FUNC_EX& func, X::XObj* pContext = nullptr) override;
		virtual XProp* CreateProp(const char* name, U_FUNC& setter, U_FUNC& getter, XObj* pContext) override;
		virtual XDict* CreateDict() override;
		virtual XList* CreateList() override;
		virtual XTensor* CreateTensor() override;
		virtual XStruct* CreateStruct(char* data,int size,bool asRef) override;
		virtual XTensorOperator* CreateTensorOperator(Tensor_OperatorHandler op,bool isUnaryOp) override;
		virtual XTensorGraph* CreateTensorGraph() override;
		virtual XSet* CreateSet() override;
		virtual XComplex* CreateComplex() override;
		virtual XObj* ConvertObjFromPointer(void* pObjectPtr) override;
		virtual const char* StringifyString(const char* str) override;
		virtual void ReleaseString(const char* str) override;
		virtual XBin* CreateBin(char* data, size_t size, bool bOwnData) override;
		virtual X::XLStream* CreateStream(const char* buf=nullptr, long long size=0) override;
		virtual X::XLStream* CreateStreamWithSameScopeSpace(X::XLStream* pRefStream, const char* buf = nullptr, long long size = 0) override;
		virtual void ReleaseStream(X::XLStream* pStream) override;
		virtual XRemoteObject* CreateRemoteObject(XProxy* proxy) override;
		virtual bool ToBytes(X::Value& input, X::Value& output) override;
		virtual bool FromBytes(X::Value& input, X::Value& output) override;
		virtual bool ConvertToBytes(X::Value& v, X::XLStream* pStream = nullptr) override;
		virtual bool ConvertFromBytes(X::Value& v, X::XLStream* pStream = nullptr) override;
		virtual bool WriteToStream(char* data, long long size, X::XLStream* pStream) override;
		virtual bool ReadFromStream(char* buffer, long long size, X::XLStream* pStream) override;
		virtual bool RunCode(const char* moduleName, const char* code, int codeSize,X::Value& retVal) override;
		virtual bool RunCodeInNonDebug(const char* moduleName, const char* code, int codeSize, X::Value& retVal) override;
		virtual bool RunCodeWithParam(const char* moduleName, const char* code, int codeSize,X::ARGS& args,X::Value& retVal) override;
		virtual bool LoadModule(const char* moduleName, const char* code, int codeSize, X::Value& objModule) override;
		virtual bool UnloadModule(X::Value objModule) override;
		virtual bool UnloadXPackage(const char* packageName) override;
		virtual bool RunModule(X::Value objModule, X::Value& retVal, bool keepModuleWithRuntime) override
		{
			X::ARGS args(0);
			return RunModule(objModule,args, retVal, keepModuleWithRuntime);
		}
		virtual bool RunModule(X::Value objModule, X::ARGS& args, X::Value& retVal, bool keepModuleWithRuntime) override;
		virtual bool IsModuleLoadedMd5(const char* md5) override;
		virtual X::Value NewModule() override;
		virtual unsigned long long RunModuleInThread(const char* moduleName, const char* code, int codeSize, X::ARGS& args, X::KWARGS& kwargs) override;
		virtual bool RunCodeLine(const char* codeLine,int codeSize,X::Value& retVal) override;
		virtual bool RunFragmentInModule(X::Value moduleObj, const char* code, int size, X::Value& retVal, int exeNum = -1) override;
		virtual const char* GetInteractiveCode() override;
		virtual long OnEvent(const char* evtName, EventHandler handler) override;
		virtual void OffEvent(const char* evtName, long Cookie) override;
		virtual Value GetAttr(const X::Value& v, const char* attrName) override;
		virtual void SetAttr(const X::Value& v, const char* attrName, X::Value& attrVal) override;
		virtual AppEventCode HandleAppEvent(int signum) override;
		virtual bool Lrpc_Listen(int port, bool blockMode) override;
		virtual bool Import(XRuntime* rt, const char* moduleName,
			const char* from, const char* thru, X::Value& objPackage) override;
		virtual bool CreateScopeWrapper(XCustomScope* pScope) override;
		virtual bool DeleteScopeWrapper(XCustomScope* pScope) override;
		virtual bool SetExpressionScope(XCustomScope* pScope, X::Value& expr) override;
		virtual bool RunExpression(X::Value& expr, X::Value& result) override;
		virtual bool CompileExpression(const char* code,int codeSize, X::Value& expr) override;
		virtual bool ExtractNativeObjectFromRemoteObject(X::Value& remoteObj, X::Value& nativeObj) override;
		virtual void RegisterUIThreadRunHandler(UI_THREAD_RUN_HANDLER handler, void* pContext) override;
		virtual UI_THREAD_RUN_HANDLER GetUIThreadRunHandler() override;
		virtual void* GetUIThreadRunContext() override;
		virtual X::Value CreateNdarray(int nd, unsigned long long* dims, int itemDataType, void* data) override;
		virtual bool PyAddImportPaths(X::Value& paths) override;
		virtual bool PyRun(const char* code, X::ARGS& args) override;
		virtual bool PyImport(XRuntime* rt, const char* moduleName,
			const char* from, const char* currentPath,X::Value& pyObj) override;
		virtual bool PyObjToValue(void* pyObj, X::Value& valObject) override;
		virtual void SetPyEngHost(void* pHost) override;
		virtual void SetDebugMode(bool bDebug) override;
		virtual void EnalbePython(bool bEnable, bool bEnablePythonDebug) override;
		virtual void EnableDebug(bool bEnable, int port=3142) override;
		virtual void* GetLogger() override;
	};
	X::XHost* CreatXHost();
	void DestoryXHost();
}