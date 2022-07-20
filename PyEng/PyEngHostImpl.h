#pragma once
#include "singleton.h"
#include "PyEngHost.h"
#include "Locker.h"
#include <unordered_map>

//trick for win32 compile to avoid using pythonnn_d.lib
#ifdef _DEBUG
#undef _DEBUG
extern "C"
{
#include "Python.h"
}
#define _DEBUG
#else
extern "C"
{
#include "Python.h"
}
#endif

class GrusPyEngHost :
	public PyEngHost,
	public Singleton<GrusPyEngHost>
{
public:
	GrusPyEngHost();
	~GrusPyEngHost();
	// Inherited via PyEngHost
	virtual void SetTrace(Python_TraceFunc func,PyEngObjectPtr args) override;

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
	virtual PyEngObjectPtr NewArray(int nd, unsigned long long* dims, int itemDataType) override;
	virtual PyEngObjectPtr Import(const char* key) override;

	virtual void Release(PyEngObjectPtr obj) override;

	virtual int AddRef(PyEngObjectPtr obj) override;

	virtual void* GetDataPtr(PyEngObjectPtr obj) override;

	virtual long long GetCount(PyEngObjectPtr objs) override;

	virtual bool GetDataDesc(PyEngObjectPtr obj, 
		int& itemDataType, int& itemSize,
		std::vector<unsigned long long>& dims, 
		std::vector<unsigned long long>& strides) override;

	virtual long long to_longlong(PyEngObjectPtr pVar) override;
	virtual PyEngObjectPtr from_longlong(long long val) override;
	virtual double to_double(PyEngObjectPtr pVar) override;
	virtual PyEngObjectPtr from_double(double val) override;
	virtual bool IsNone(PyEngObjectPtr obj) override;
	virtual bool IsDict(PyEngObjectPtr obj) override;
	virtual bool DictContain(PyEngObjectPtr dict,std::string& strKey) override;
	virtual bool IsArray(PyEngObjectPtr obj) override;
	virtual bool IsList(PyEngObjectPtr obj) override;
	virtual PyEngObjectPtr GetDictKeys(PyEngObjectPtr obj) override;
	virtual const char* GetObjectType(PyEngObjectPtr obj) override;
	virtual PyEngObjectPtr GetDictItems(PyEngObjectPtr dict) override;
	virtual PyEngObjectPtr GetPyNone() override;
	virtual PyEngObjectPtr GetGlobals() override;
	virtual PyEngObjectPtr GetLocals() override;
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
};
