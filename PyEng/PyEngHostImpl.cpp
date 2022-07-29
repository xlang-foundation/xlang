#include "PyEngHostImpl.h"
#include "utility.h"
#include "PyEngObject.h"
#include <iostream>

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
	PyObject* pRetOb = Py_None;
	if (pOb != nullptr && PyDict_Check(pOb))
	{
		PyObject* pObKey = PyUnicode_FromString(key);
		pRetOb = PyDict_GetItem(pOb, pObKey);//Borrowed reference
		Py_IncRef(pRetOb);
		Py_DecRef(pObKey);
	}
	else
	{
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
	bool bOK = false;
	if (PyDict_Check((PyObject*)container))
	{
		bOK = PyDict_SetItem((PyObject*)container, (PyObject*)key, (PyObject*)val);
	}
	return bOK;
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

PyEngObjectPtr GrusPyEngHost::NewArray(int nd, unsigned long long* dims, int itemDataType)
{
	SURE_NUMPY_API();

	auto aryData = (PyArrayObject*)PyArray_SimpleNew(
		nd,
		(npy_intp *)dims,
		itemDataType);
	return (PyEngObjectPtr)aryData;
}
void GrusPyEngHost::SetTrace(Python_TraceFunc func,
	PyEngObjectPtr args)
{
	PyEval_SetTrace((Py_tracefunc)func, (PyObject*)args);
}
PyEngObjectPtr GrusPyEngHost::Import(const char* key)
{
	return (PyEngObjectPtr)PyImport_ImportModule(key);
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
	std::vector<unsigned long long>& dims,
	std::vector<unsigned long long>& strides)
{
	SURE_NUMPY_API();
	PyObject* pOb = (PyObject*)obj;
	if (PyArray_Check(pOb))
	{
		PyArrayObject* pArrayOb = (PyArrayObject*)pOb;
		PyArray_Descr* pDesc = PyArray_DTYPE(pArrayOb);
		itemSize = (int)PyArray_ITEMSIZE(pArrayOb);
		itemDataType = pDesc->type_num;
		for (int i = 0; i < PyArray_NDIM(pArrayOb); i++)
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
	std::string& strKey)
{
	return PyDict_Check((PyObject*)dict) &&
		PyDict_Contains((PyObject*)dict, (PyObject*)PyEng::Object(strKey).ref());
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
