#ifndef _PyEngHost_H_
#define _PyEngHost_H_

#include <vector>
#include <string>

//all function,if return PyEngObjectPtr will hold a new reference
typedef void* PyEngObjectPtr;
typedef int (*Python_TraceFunc)(
	PyEngObjectPtr self,
	PyEngObjectPtr frame,
	int event,
	PyEngObjectPtr args);

class PyEngHost
{
public:
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
	virtual PyEngObjectPtr NewArray(int nd, unsigned long long* dims, int itemDataType) = 0;
	virtual void* GetDataPtr(PyEngObjectPtr obj) = 0;
	virtual bool GetDataDesc(PyEngObjectPtr obj, 
		int& itemDataType,int& itemSize,
		std::vector<unsigned long long>& dims,
		std::vector<unsigned long long>& strides) = 0;
	virtual PyEngObjectPtr Import(const char* key) = 0;
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
	virtual bool DictContain(PyEngObjectPtr dict,std::string& strKey) = 0;
	virtual PyEngObjectPtr GetPyNone() = 0;
	virtual PyEngObjectPtr CreateByteArray(const char* buf, long long size) = 0;
	virtual PyEngObjectPtr GetGlobals() = 0;
	virtual PyEngObjectPtr GetLocals() = 0;
};

extern PyEngHost* g_pPyHost;

#endif//_PyEngHost_H_

