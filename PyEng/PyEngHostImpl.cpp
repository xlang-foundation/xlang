#include "PyEngHostImpl.h"
#include "utility.h"
#include "PyFunc.h"
#include <iostream>
#include <sstream>
#include "port.h"

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

//each cpp file has to call this line
extern "C"
{
#include "numpy/ndarrayobject.h"
#include "numpy/arrayobject.h"
}


static void LoadNumpy()
{
	if (PyArray_API == NULL)
	{
		_import_array();
	}
}
#define SURE_NUMPY_API() LoadNumpy()


GrusPyEngHost::GrusPyEngHost()
{

}
GrusPyEngHost::~GrusPyEngHost()
{

}
PyEngObjectPtr GrusPyEngHost::CreatePythonFuncProxy(
	void* realFuncObj, void* pContext)
{
	PyObject* pRetObj = CreateFuncWrapper(this, realFuncObj, pContext);
	return (PyEngObjectPtr)pRetObj;
}
int GrusPyEngHost::to_int(PyEngObjectPtr pVar)
{
	return (int)PyLong_AsLong((PyObject*)pVar);
}

PyEngObjectPtr GrusPyEngHost::from_int(int val)
{
	PyObject* pOb = PyLong_FromLong(val);
	return (PyEngObjectPtr)pOb;
}

long long GrusPyEngHost::to_longlong(PyEngObjectPtr pVar)
{
	return PyLong_AsLongLong((PyObject*)pVar);

}

PyEngObjectPtr GrusPyEngHost::from_longlong(long long val)
{
	PyObject* pOb = PyLong_FromLongLong(val);
	return (PyEngObjectPtr)pOb;
}

double GrusPyEngHost::to_double(PyEngObjectPtr pVar)
{
	return (double)PyFloat_AsDouble((PyObject*)pVar);
}

PyEngObjectPtr GrusPyEngHost::from_double(double val)
{
	return (PyEngObjectPtr)PyFloat_FromDouble((double)val);
}

bool GrusPyEngHost::IsNone(PyEngObjectPtr obj)
{
	return ((PyObject*)obj == Py_None);
}

float GrusPyEngHost::to_float(PyEngObjectPtr pVar)
{
	return (float)PyFloat_AsDouble((PyObject*)pVar);
}

PyEngObjectPtr GrusPyEngHost::from_float(float val)
{
	return (PyEngObjectPtr)PyFloat_FromDouble((double)val);
}
std::string ConvertFromObject(PyObject* pOb)
{
	std::string strVal;
	if (pOb == nullptr)
	{
		return strVal;
	}
	if (PyUnicode_Check(pOb))
	{
		strVal = PyUnicode_AsUTF8(pOb);
	}
	else
	{
		PyObject* strOb = PyObject_Repr(pOb);
		strVal = PyUnicode_AsUTF8(strOb);
		Py_DECREF(strOb);

	}
	return strVal;
}

const char* GrusPyEngHost::to_str(PyEngObjectPtr pVar)
{
	std::string str =  ConvertFromObject((PyObject*)pVar);
	char* newStr = new char[str.size()+1];
	memcpy(newStr, str.c_str(), str.size()+1);
	return newStr;
}

PyEngObjectPtr GrusPyEngHost::from_str(const char* val)
{
	return (PyEngObjectPtr*)PyUnicode_FromString(val);
}

long long GrusPyEngHost::GetCount(PyEngObjectPtr objs)
{
	PyObject* pOb = (PyObject*)objs;
	if (pOb == nullptr)
	{
		return 0;
	}
	long long cnt = 1;
	if (PyTuple_Check(pOb))
	{
		cnt = PyTuple_GET_SIZE(pOb);
	}
	else if (PyDict_Check(pOb))
	{
		cnt = PyDict_GET_SIZE(pOb);
	}
	else if (PyList_Check(pOb))
	{
		cnt = PyList_GET_SIZE(pOb);
	}
	else if (PyByteArray_Check(pOb))
	{
		cnt = PyByteArray_Size(pOb);
	}
	//else if (IsArray(pOb))
	//{
	//	
	//}
	return cnt;
}

PyEngObjectPtr GrusPyEngHost::Get(PyEngObjectPtr objs, int idx)
{
	PyObject* pOb = (PyObject*)objs;
	PyObject* pRetOb = Py_None;
	if (PyTuple_Check(pOb))
	{
		auto size = PyTuple_Size(pOb);
		if (idx < size)
		{
			pRetOb = PyTuple_GetItem(pOb, idx);
		}
	}
	else if (PyList_Check(pOb))
	{
		auto size = PyList_Size(pOb);
		if (idx < size)
		{
			pRetOb = PyList_GetItem(pOb, idx);
		}
	}
	else if (PyDict_Check(pOb))
	{
		PyObject* pObKey = PyLong_FromLong(idx);
		pRetOb = PyDict_GetItem(pOb, pObKey);
		Py_DecRef(pObKey);
	}
	else
	{
		//call 
		auto func = Get(pOb, "__getitem__");
		PyObject* pObIndex = PyLong_FromLong(idx);
		auto retOb = Call(func, 1, (PyEngObjectPtr*)& pObIndex);
		Py_DecRef(pObIndex);
		Py_DecRef((PyObject*)func);
		pRetOb = (PyObject*)retOb;
	}
	//all come from Borrowed reference,so add one
	Py_IncRef(pRetOb);
	return (PyEngObjectPtr)pRetOb;
}

int GrusPyEngHost::Set(PyEngObjectPtr objs, int idx, PyEngObjectPtr val)
{	
	PyObject* pOb = (PyObject*)objs;
	int retVal = 0;
	if (PyList_Check(pOb))
	{
		retVal = PyList_SetItem(pOb, idx, (PyObject*)val);
	}
	else if (PyTuple_Check(pOb))
	{
		retVal = PyTuple_SetItem(pOb, idx, (PyObject*)val);
	}
	else if (PyDict_Check(pOb))
	{
		PyObject* pObKey = PyLong_FromLong(idx);
		retVal = PyDict_SetItem(pOb,pObKey, (PyObject*)val);
		Py_DecRef(pObKey);
	}
	return retVal;
}

void GrusPyEngHost::Free(const char* sz)
{
	delete sz;
}

PyEngObjectPtr GrusPyEngHost::Get(PyEngObjectPtr objs, const char* key)
{
	PyObject* pOb = (PyObject*)objs;
	PyObject* pRetOb = nullptr;
	if (pOb != nullptr && PyDict_Check(pOb))
	{
		PyObject* pObKey = PyUnicode_FromString(key);
		pRetOb = PyDict_GetItem(pOb, pObKey);//Borrowed reference
		if (pRetOb)
		{
			Py_IncRef(pRetOb);
		}
		Py_DecRef(pObKey);
	}
	//try again if key is an attribute
	if(pRetOb == nullptr)
	{
		pRetOb = Py_None;
		if (strchr(key,'.') == nullptr)
		{
			if (pOb == nullptr)
			{
				pRetOb = (PyObject*)Import(key);//New reference
			}
			else
			{
				pRetOb = PyObject_GetAttrString(pOb, key);//New reference
			}
		}
		else
		{
			std::string strKey(key);
			auto keys = split(strKey, '.');
			int i = 0;
			if (pOb)
			{
				Py_IncRef(pOb);
			}
			else
			{
				pOb = (PyObject*)Import(keys[0].c_str());
				if (pOb == nullptr)
				{
					PyErr_PrintEx(0);
				}
				i = 1;
			}
			for (;i<(int)keys.size();i++)
			{
				auto k = keys[i];
				//New reference
				pRetOb = PyObject_GetAttrString(pOb, k.c_str());
				if (pRetOb != nullptr)
				{
					Py_DecRef(pOb);
					pOb = pRetOb;
				}
				else
				{
					Py_DecRef(pOb);
					break;
				}
			}
		}
	}
	if (pRetOb == nullptr)
	{
		pRetOb = Py_None;
		Py_IncRef(pRetOb);
	}
	return (PyEngObjectPtr)pRetOb;
}

PyEngObjectPtr GrusPyEngHost::Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args)
{
	PyObject* pRetOb = Py_None;
	PyObject* pCallOb = (PyObject*)obj;
	if (!PyCallable_Check(pCallOb))
	{
		Py_IncRef(pRetOb);
		return pRetOb;
	}
	PyObject* pObArgs = PyTuple_New(argNum);
	for (int i = 0; i < argNum; i++)
	{
		PyObject* arg = (PyObject*)args[i];//args already hold one refcount
		Py_IncRef(arg);
		PyTuple_SetItem(pObArgs,i, arg);
	}
	pRetOb = PyObject_CallObject(pCallOb, pObArgs);
	Py_DECREF(pObArgs);

	return (PyEngObjectPtr)pRetOb;
}
PyEngObjectPtr GrusPyEngHost::Call(PyEngObjectPtr obj, PyEngObjectPtr args, PyEngObjectPtr kwargs)
{
	PyObject* pRetOb = Py_None;
	PyObject* pCallOb = (PyObject*)obj;
	if (!PyCallable_Check(pCallOb))
	{
		Py_IncRef(pRetOb);
		return pRetOb;
	}
	pRetOb = PyObject_Call(pCallOb,(PyObject*)args, (PyObject*)kwargs);
	return (PyEngObjectPtr)pRetOb;
}
PyEngObjectPtr GrusPyEngHost::Call(PyEngObjectPtr obj, int argNum, PyEngObjectPtr* args, PyEngObjectPtr kwargs)
{
	PyObject* pRetOb = Py_None;
	PyObject* pCallOb = (PyObject*)obj;
	if (!PyCallable_Check(pCallOb))
	{
		Py_IncRef(pRetOb);
		return pRetOb;
	}
	PyObject* pObArgs = PyTuple_New(argNum);
	for (int i = 0; i < argNum; i++)
	{
		PyObject* arg = (PyObject*)args[i];//args already hold one refcount
		Py_IncRef(arg);//?
		PyTuple_SetItem(pObArgs, i, arg);
	}
	PyObject* pkwargs = (PyObject*)kwargs;

	pRetOb = PyObject_Call(pCallOb, pObArgs, pkwargs);
	Py_DECREF(pObArgs);

	return (PyEngObjectPtr)pRetOb;
}

bool GrusPyEngHost::ContainKey(PyEngObjectPtr container, PyEngObjectPtr key)
{
	bool bOK = false;
	if (PyDict_Check((PyObject*)container))
	{
		bOK = PyDict_Contains((PyObject*)container, (PyObject*)key);
	}
	return bOK;
}

bool GrusPyEngHost::KVSet(PyEngObjectPtr container, PyEngObjectPtr key, PyEngObjectPtr val)
{
	int  ret = 0;
	if (PyDict_Check((PyObject*)container))
	{
		ret = PyDict_SetItem((PyObject*)container, (PyObject*)key, (PyObject*)val);
	}
	else
	{
		ret = PyObject_SetAttrString((PyObject*)container, PyUnicode_AsUTF8((PyObject*)key), (PyObject*)val);
	}
	return ret>=0;
}
PyEngObjectPtr GrusPyEngHost::NewTuple(long long size)
{
	return (PyEngObjectPtr)PyTuple_New(size);
}
PyEngObjectPtr GrusPyEngHost::NewList(long long size)
{
	return (PyEngObjectPtr)PyList_New(size);
}

PyEngObjectPtr GrusPyEngHost::NewDict()
{
	return (PyEngObjectPtr)PyDict_New();
}

PyEngObjectPtr GrusPyEngHost::NewArray(int nd, unsigned long long* dims, int itemDataType,void* data)
{
	SURE_NUMPY_API();
	
	PyEngObjectPtr aryData = nullptr;
	if (data == nullptr)
	{
		aryData = (PyArrayObject*)PyArray_SimpleNew(
			nd, (npy_intp*)dims, itemDataType);
	}
	else
	{
		aryData = (PyArrayObject*)PyArray_SimpleNewFromData(
			nd,(npy_intp*)dims,itemDataType, data);
	}
	return aryData;
}
void GrusPyEngHost::SetTrace(Python_TraceFunc func,
	PyEngObjectPtr args)
{
	PyEval_SetTrace((Py_tracefunc)func, (PyObject*)args);
}
PyEngObjectPtr GrusPyEngHost::Import(const char* key)
{
	auto* pOb = PyImport_ImportModule(key);
	return (PyEngObjectPtr)pOb;
}


//deal with import cv2 case, emebed Python bugs: need to preload cv2.pyd at least in Windows
PyEngObjectPtr GrusPyEngHost::ImportWithPreloadRequired(const char* key)
{
	auto load = [key](std::string& path) {
		std::string fullPath = path + Path_Sep_S + key+ Path_Sep_S+key;
#if (WIN32)
		fullPath += ".pyd";
#else
		fullPath += ".so";
#endif
		return LOADLIB(fullPath.c_str());
	};
	//check if there is pyd file in site-package folder
	PyObject* pSiteModule = PyImport_ImportModule("site");
	if (!pSiteModule) 
	{
		PyErr_Print();
		return nullptr;
	}
	PyObject* pSitePackagesPath = PyObject_CallMethod(pSiteModule, "getsitepackages", nullptr);
	if (!pSitePackagesPath)
	{
		Py_DECREF(pSiteModule);
		PyErr_Print();
		return nullptr;
	}
	void* handleLoaded = nullptr;
	// Enumerate and convert the paths to strings
	Py_ssize_t listSize = PyList_Size(pSitePackagesPath);
	for (Py_ssize_t i = 0; i < listSize; ++i) 
	{
		PyObject* pPathItem = PyList_GetItem(pSitePackagesPath, i);
		std::string sitePackagesPath = ConvertFromObject(pPathItem);
		auto handle = load(sitePackagesPath);
		if (handle)
		{
			handleLoaded = handle;
			break;
		}
	}
	Py_DECREF(pSiteModule);
	Py_DECREF(pSitePackagesPath);
	auto* pOb = PyImport_ImportModule(key);
	if (handleLoaded)
	{
		UNLOADLIB(handleLoaded);
	}
	return (PyEngObjectPtr)pOb;
}
//for this use case
//from moduleName import sub1,sub2....
//in fact, sub1,sub2... are the from list
//and the from part above is the module name

bool GrusPyEngHost::ImportWithFromList(
	const char* moduleName,
	X::Port::vector<const char*>& fromList,
	X::Port::vector<PyEngObjectPtr>& subs)
{
	PyObject* pFromArgs = PyTuple_New(fromList.size());
	for (auto& from : fromList)
	{
		auto* pFromOb = PyUnicode_FromString(from);
		PyTuple_SetItem(pFromArgs, 0, pFromOb);
	}
	auto* pModule = PyImport_ImportModuleLevel(moduleName,
		Py_None, Py_None, pFromArgs,0);
	Py_DecRef(pFromArgs);
	bool bOK = false;
	if (pModule)
	{
		bOK = true;
		subs.resize((int)fromList.size());
		for (auto& from : fromList)
		{
			auto* pSubOb = PyObject_GetAttrString(pModule, from);//New reference
			subs.push_back(pSubOb);//if it is null, also pust into list for caller
		}
	}
	Py_DecRef(pModule);
	return bOK;
}

void GrusPyEngHost::Release(PyEngObjectPtr obj)
{
	PyObject* pOb = (PyObject*)obj;
	Py_DecRef(pOb);
}

int GrusPyEngHost::AddRef(PyEngObjectPtr obj)
{
	PyObject* pOb = (PyObject*)obj;
	Py_IncRef(pOb);
	return (int)pOb->ob_refcnt;
}

void* GrusPyEngHost::GetDataPtr(PyEngObjectPtr obj)
{
	SURE_NUMPY_API();
	PyObject* pOb = (PyObject*)obj;
	if (PyArray_Check(pOb))
	{
		PyArrayObject* pArrayOb = (PyArrayObject*)pOb;
		return PyArray_DATA(pArrayOb);
	}
	else if (PyByteArray_Check(pOb))
	{
		return (void*)PyByteArray_AsString(pOb);
	}
	return nullptr;
}

bool GrusPyEngHost::GetDataDesc(PyEngObjectPtr obj,
	int& itemDataType, int& itemSize,
	X::Port::vector<unsigned long long>& dims,
	X::Port::vector<unsigned long long>& strides)
{
	SURE_NUMPY_API();
	PyObject* pOb = (PyObject*)obj;
	if (PyArray_Check(pOb))
	{
		PyArrayObject* pArrayOb = (PyArrayObject*)pOb;
		PyArray_Descr* pDesc = PyArray_DTYPE(pArrayOb);
		itemSize = (int)PyArray_ITEMSIZE(pArrayOb);
		itemDataType = pDesc->type_num;
		auto dimSize = PyArray_NDIM(pArrayOb);
		dims.resize(dimSize);
		strides.resize(dimSize);
		for (int i = 0; i < dimSize; i++)
		{
			dims.push_back(PyArray_DIM(pArrayOb,i));
			strides.push_back(PyArray_STRIDE(pArrayOb,i));
		}
		return true;
	}
	else
	{
		return false;
	}
}
bool GrusPyEngHost::IsDict(PyEngObjectPtr obj)
{
	return PyDict_Check((PyObject*)obj);
}
bool GrusPyEngHost::DictContain(PyEngObjectPtr dict,
	const char* strKey)
{
	PyObject* keyOb = (PyObject*)g_pPyHost->from_str(strKey);
	bool bContain = PyDict_Check((PyObject*)dict) &&
		PyDict_Contains((PyObject*)dict, keyOb);
	g_pPyHost->Release(keyOb);
	return bContain;
}
bool GrusPyEngHost::IsArray(PyEngObjectPtr obj)
{
	SURE_NUMPY_API();
	return PyArray_Check((PyObject*)obj);
}
bool GrusPyEngHost::IsList(PyEngObjectPtr obj)
{
	return PyList_Check((PyObject*)obj);
}
PyEngObjectPtr GrusPyEngHost::GetDictKeys(PyEngObjectPtr obj)
{
	return PyDict_Keys((PyObject*)obj);
}


const char* GrusPyEngHost::GetObjectType(PyEngObjectPtr obj)
{
	PyObject* pOb = (PyObject*)obj;
	return Py_TYPE(pOb)->tp_name;
}

PyEngObjectPtr GrusPyEngHost::GetDictItems(PyEngObjectPtr dict)
{
	return PyDict_Items((PyObject*)dict);
}

PyEngObjectPtr GrusPyEngHost::GetPyNone()
{
	PyObject* pOb = Py_None;
	Py_IncRef(pOb);
	return pOb;
}

PyEngObjectPtr GrusPyEngHost::GetLocals()
{
	return (PyEngObjectPtr)PyEval_GetLocals();
}

PyEngObjectPtr GrusPyEngHost::GetGlobals()
{
	return (PyEngObjectPtr)PyEval_GetGlobals();
}

PyEngObjectPtr GrusPyEngHost::CreateByteArray(const char* buf, long long size)
{
	return (PyEngObjectPtr)PyByteArray_FromStringAndSize(buf,size);
}

bool GrusPyEngHost::IsBool(PyEngObjectPtr obj)
{
	return PyBool_Check((PyObject*)obj);
}

bool GrusPyEngHost::IsLong(PyEngObjectPtr obj)
{
	return PyLong_Check((PyObject*)obj);
}

bool GrusPyEngHost::IsDouble(PyEngObjectPtr obj)
{
	return PyFloat_Check((PyObject*)obj);
}

bool GrusPyEngHost::IsString(PyEngObjectPtr obj)
{
	return PyUnicode_Check((PyObject*)obj);
}

bool GrusPyEngHost::IsTuple(PyEngObjectPtr obj)
{
	return PyTuple_Check((PyObject*)obj);
}

bool GrusPyEngHost::IsSet(PyEngObjectPtr obj)
{
	return PySet_Check((PyObject*)obj);
}
PyEngObjectPtr GrusPyEngHost::GetIter(PyEngObjectPtr obj)
{
	return (PyEngObjectPtr)PyObject_GetIter((PyObject*)obj);
}
PyEngObjectPtr GrusPyEngHost::GetIterNext(PyEngObjectPtr iterator)
{
	return (PyEngObjectPtr)PyIter_Next((PyObject*)iterator);
}
bool GrusPyEngHost::EnumDictItem(PyEngObjectPtr dict, long long& pos,
	PyEngObjectPtr& key, PyEngObjectPtr& val)
{
	Py_ssize_t pyPos = pos;
	PyObject* pyKey = nullptr;
	PyObject* pyVal = nullptr;
	bool bOK = PyDict_Next((PyObject*)dict, &pyPos, &pyKey, &pyVal);
	key = (PyEngObjectPtr)pyKey;
	val = (PyEngObjectPtr)pyVal;
	pos = pyPos;
	return bOK;
}

bool GrusPyEngHost::CallReleaseForTupleItems(PyEngObjectPtr tuple)
{
	if (!PyTuple_Check((PyObject*)tuple))
	{
		return false;
	}
	std::ostringstream oss;
	Py_ssize_t size = PyTuple_Size((PyObject*)tuple);
	oss << "CallReleaseForTupleItems,tuple size:" << size;
	for (Py_ssize_t i = 0; i < size; i++)
	{
		PyObject* pOb = PyTuple_GetItem((PyObject*)tuple, i);
		int cnt = (int)pOb->ob_refcnt;
		oss <<"i="<<i<<",ref_count,before release="<<cnt<<",";
		Py_DecRef(pOb);
	}
	std::string result = oss.str();
	std::cout << result << std::endl;
	return true;
}

bool GrusPyEngHost::Exec(const char* code, PyEngObjectPtr args)
{
	PyObject* argsTuple = (PyObject*)args;
	PyObject* sysModule = PyImport_ImportModule("sys");
	if (!sysModule) 
	{
		return false;
	}

	PyObject* sysArgv = PyObject_GetAttrString(sysModule, "argv");
	if (!sysArgv || !PyList_Check(sysArgv)) 
	{
		Py_DECREF(sysModule);
		return false;
	}
	Py_ssize_t n = PyTuple_Size(argsTuple);
	for (Py_ssize_t i = 0; i < n; i++) 
	{
		PyObject* item = PyTuple_GetItem(argsTuple, i); // Borrowed reference, no need to decref
		if (PyList_Append(sysArgv, item) < 0) 
		{
			Py_DECREF(sysModule);
			Py_DECREF(sysArgv);
			return false;
		}
	}
	int ret = PyRun_SimpleString(code);
	Py_DECREF(sysModule);
	Py_DECREF(sysArgv);

	return (ret ==0);
}
