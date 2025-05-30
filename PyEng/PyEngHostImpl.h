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
#include "singleton.h"
#include "PyEngHost.h"
#include "xlang.h"
#include "PythonThreadPool.h"

class GrusPyEngHost :
	public PyEngHost,
	public Singleton<GrusPyEngHost>
{
	Xlang_CallFunc m_xlang_call_func = nullptr;
	PythonThreadPool m_pyTaskPool;
	PyThreadState* m_mainThreadState = nullptr;
public:
	GrusPyEngHost();
	~GrusPyEngHost();
	FORCE_INLINE Xlang_CallFunc GetXlangCallFunc()
	{
		return m_xlang_call_func;
	}
	FORCE_INLINE void SetPyThreadState(PyThreadState* s)
	{
		m_mainThreadState = s;
	}
	FORCE_INLINE PyThreadState* GetPyThreadState()
	{
		return m_mainThreadState;
	}
	// Inherited via PyEngHost
	virtual void SetXlangCallFunc(Xlang_CallFunc xlangCall) override
	{
		m_xlang_call_func = xlangCall;
	}
	virtual PyEngObjectPtr CreatePythonFuncProxy(
		void* realFuncObj, void* pContext) override;
	virtual void SetTrace(Python_TraceFunc func,
		PyEngObjectPtr args) override;

	virtual int to_int(PyEngObjectPtr pVar) override;

	virtual PyEngObjectPtr from_int(int val) override;

	virtual float to_float(PyEngObjectPtr pVar) override;

	virtual PyEngObjectPtr from_float(float val) override;

	virtual const char* to_str(PyEngObjectPtr pVar) override;

	virtual PyEngObjectPtr from_str(const char* val) override;
	virtual PyEngObjectPtr Get(PyEngObjectPtr objs, int idx) override;
	virtual int Set(PyEngObjectPtr objs, int idx, PyEngObjectPtr val) override;
	virtual void Free(const char* sz) override;

	virtual PyEngObjectPtr Get(PyEngObjectPtr objs, const char* key) override;

	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args) override;
	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args, PyEngObjectPtr kwargs) override;
	virtual PyEngObjectPtr Call(PyEngObjectPtr obj, PyEngObjectPtr args, PyEngObjectPtr kwargs) override;
	virtual bool ContainKey(PyEngObjectPtr container, PyEngObjectPtr key) override;
	virtual bool KVSet(PyEngObjectPtr container, PyEngObjectPtr key, PyEngObjectPtr val) override;

	virtual PyEngObjectPtr NewTuple (long long size) override;
	virtual PyEngObjectPtr NewList(long long size) override;
	virtual PyEngObjectPtr NewDict() override;
	virtual PyEngObjectPtr NewArray(int nd, unsigned long long* dims, int itemDataType,void* data) override;
	virtual PyEngObjectPtr Import(const char* key) override;
	virtual PyEngObjectPtr ImportWithPreloadRequired(const char* key) override;
	virtual bool ImportWithFromList(const char* moduleName,
		X::Port::vector<const char*>& fromList, 
		X::Port::vector<PyEngObjectPtr>& subs) override;
	virtual void Release(PyEngObjectPtr obj) override;

	virtual int AddRef(PyEngObjectPtr obj) override;

	virtual void* GetDataPtr(PyEngObjectPtr obj) override;

	virtual long long GetCount(PyEngObjectPtr objs) override;

	virtual bool GetDataDesc(PyEngObjectPtr obj, 
		int& itemDataType, int& itemSize,
		X::Port::vector<unsigned long long>& dims, 
		X::Port::vector<unsigned long long>& strides) override;

	virtual long long to_longlong(PyEngObjectPtr pVar) override;
	virtual PyEngObjectPtr from_longlong(long long val) override;
	virtual double to_double(PyEngObjectPtr pVar) override;
	virtual PyEngObjectPtr from_double(double val) override;
	virtual bool IsNone(PyEngObjectPtr obj) override;
	virtual bool IsDict(PyEngObjectPtr obj) override;
	virtual bool DictContain(PyEngObjectPtr dict,const char* strKey) override;
	virtual PyEngObjectPtr GetIter(PyEngObjectPtr obj) override;
	virtual PyEngObjectPtr GetIterNext(PyEngObjectPtr iterator) override;
	virtual bool IsArray(PyEngObjectPtr obj) override;
	virtual bool IsList(PyEngObjectPtr obj) override;
	virtual PyEngObjectPtr GetDictKeys(PyEngObjectPtr obj) override;
	virtual const char* GetObjectType(PyEngObjectPtr obj) override;
	virtual PyEngObjectPtr GetDictItems(PyEngObjectPtr dict) override;
	virtual PyEngObjectPtr GetPyNone() override;
	virtual PyEngObjectPtr GetGlobals() override;
	virtual PyEngObjectPtr GetLocals() override;
	virtual void InitPythonThreads() override;
	virtual int GilLock() override;
	virtual void GilUnlock(int state) override;
	virtual void SubmitPythonTask(const std::function<void()>& task) override;
	virtual void AddImportPaths(const char* path) override;
private:
	virtual PyEngObjectPtr CreateByteArray(const char* buf, long long size) override;

	// Inherited via PyEngHost
	virtual bool IsBool(PyEngObjectPtr obj) override;
	virtual bool IsLong(PyEngObjectPtr obj) override;
	virtual bool IsDouble(PyEngObjectPtr obj) override;
	virtual bool IsString(PyEngObjectPtr obj) override;

	// Inherited via PyEngHost
	virtual bool IsTuple(PyEngObjectPtr obj) override;
	virtual bool IsSet(PyEngObjectPtr obj) override;

	// Inherited via PyEngHost
	virtual bool EnumDictItem(PyEngObjectPtr dict, long long& pos, 
		PyEngObjectPtr& key, PyEngObjectPtr& val) override;

	virtual bool CallReleaseForTupleItems(PyEngObjectPtr tuple) override;
	virtual bool Exec(const char* code, PyEngObjectPtr args) override;

	// Inherited via PyEngHost
	X::Value to_xvalue(PyEngObjectPtr pVar) override;
	PyEngObjectPtr from_xvalue(const X::Value& val) override;
};
