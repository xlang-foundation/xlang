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

#ifndef _PyEngHost_H_
#define _PyEngHost_H_

#include "xport.h"
#include "xlang.h"
#include <functional>

//all function,if return PyEngObjectPtr will hold a new reference
typedef void* PyEngObjectPtr;
typedef int (*Python_TraceFunc)(
	PyEngObjectPtr self,
	PyEngObjectPtr frame,
	int event,
	PyEngObjectPtr args);

typedef PyEngObjectPtr (*Xlang_CallFunc)(
	void* realFuncObj,
	void* pContext,
	PyEngObjectPtr args,
	PyEngObjectPtr kwargs);

class PyEngHost
{
public:
	virtual void SetXlangCallFunc(Xlang_CallFunc xlangCall) = 0;
	virtual PyEngObjectPtr CreatePythonFuncProxy(void* realFuncObj,void* pContext) = 0;
	virtual void SetTrace(Python_TraceFunc func,PyEngObjectPtr args) = 0;
	virtual int to_int(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_int(int val) = 0;
	virtual long long to_longlong(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_longlong(long long val) = 0;
	virtual float to_float(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_float(float val) = 0;
	virtual double to_double(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_double(double val) = 0;
	virtual const char* to_str(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_str(const char* val) = 0;
	virtual X::Value to_xvalue(PyEngObjectPtr pVar) = 0;
	virtual PyEngObjectPtr from_xvalue(const X::Value& val) = 0;

	virtual long long GetCount(PyEngObjectPtr objs) = 0;
	virtual PyEngObjectPtr Get(PyEngObjectPtr objs, int idx) = 0;
	virtual int Set(PyEngObjectPtr objs, int idx, PyEngObjectPtr val) = 0;
	virtual PyEngObjectPtr Get(PyEngObjectPtr objs, const char* key) = 0;
	virtual bool ContainKey(PyEngObjectPtr container, PyEngObjectPtr key) = 0;
	virtual bool KVSet(PyEngObjectPtr container, PyEngObjectPtr key, PyEngObjectPtr val) = 0;
	virtual void Free(const char* sz) = 0;
	virtual int AddRef(PyEngObjectPtr obj) = 0;
	virtual void Release(PyEngObjectPtr obj) = 0;
	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args) = 0;
	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args, PyEngObjectPtr kwargs) = 0;
	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, PyEngObjectPtr args, PyEngObjectPtr kwargs) = 0;
	virtual PyEngObjectPtr NewList(long long size) = 0;
	virtual PyEngObjectPtr NewTuple(long long size) = 0;
	virtual PyEngObjectPtr NewDict() = 0;
	virtual PyEngObjectPtr NewArray(int nd, unsigned long long* dims, int itemDataType,void* data) = 0;
	virtual void* GetDataPtr(PyEngObjectPtr obj) = 0;
	virtual bool GetDataDesc(PyEngObjectPtr obj, 
		int& itemDataType,int& itemSize,
		X::Port::vector<unsigned long long>& dims,
		X::Port::vector<unsigned long long>& strides) = 0;
	virtual PyEngObjectPtr Import(const char* key) = 0;
	virtual PyEngObjectPtr ImportWithPreloadRequired(const char* key) = 0;
	virtual bool ImportWithFromList(const char* moduleName,
		X::Port::vector<const char*>& fromList, X::Port::vector<PyEngObjectPtr>& subs) = 0;
	virtual bool IsNone(PyEngObjectPtr obj) = 0;
	virtual bool IsBool(PyEngObjectPtr obj) = 0;
	virtual bool IsLong(PyEngObjectPtr obj) = 0;
	virtual bool IsDouble(PyEngObjectPtr obj) = 0;
	virtual bool IsString(PyEngObjectPtr obj) = 0;
	virtual bool IsDict(PyEngObjectPtr obj) = 0;
	virtual bool IsTuple(PyEngObjectPtr obj) = 0;
	virtual bool IsSet(PyEngObjectPtr obj) = 0;
	virtual bool IsList(PyEngObjectPtr obj) = 0;
	virtual bool IsArray(PyEngObjectPtr obj) = 0;
	virtual PyEngObjectPtr GetDictKeys(PyEngObjectPtr obj) = 0;
	virtual const char* GetObjectType(PyEngObjectPtr obj) = 0;
	virtual PyEngObjectPtr GetDictItems(PyEngObjectPtr dict) = 0;
	virtual bool EnumDictItem(PyEngObjectPtr dict,long long& pos, PyEngObjectPtr& key, PyEngObjectPtr& val) = 0;
	virtual bool DictContain(PyEngObjectPtr dict,const char* strKey) = 0;
	virtual PyEngObjectPtr GetIter(PyEngObjectPtr obj) = 0;
	virtual PyEngObjectPtr GetIterNext(PyEngObjectPtr iterator) = 0;
	virtual PyEngObjectPtr GetPyNone() = 0;
	virtual PyEngObjectPtr CreateByteArray(const char* buf, long long size) = 0;
	virtual PyEngObjectPtr GetGlobals() = 0;
	virtual PyEngObjectPtr GetLocals() = 0;
	virtual bool CallReleaseForTupleItems(PyEngObjectPtr tuple) = 0;
	virtual bool Exec(const char* code, PyEngObjectPtr args) = 0;
	virtual void InitPythonThreads() = 0;
	virtual int GilLock() = 0;
	virtual void GilUnlock(int state) = 0;
	virtual void SubmitPythonTask(const std::function<void()>& task) = 0;
	virtual void AddImportPaths(const char* path) = 0;
};

extern PyEngHost* g_pPyHost;

#endif//_PyEngHost_H_

