#pragma once
#include "xlang.h"
#include "xhost.h"
#include "PyGILState.h"

extern PyObject* CreateXlangObjectWrapper(X::Value& realObj);
extern X::Value CheckXlangObjectAndConvert(PyObject* obj);

class PyObjectXLangConverter {
public:
    static X::Value ConvertToXValue(PyObject* obj) {
        MGil gil;
        if (PyLong_Check(obj)) {
            return X::Value(PyLong_AsLong(obj));
        }
        else if (PyFloat_Check(obj)) {
            return X::Value(PyFloat_AsDouble(obj));
        }
        else if (PyUnicode_Check(obj)) {
            return X::Value(PyUnicode_AsUTF8(obj));
        }
        else if (PyList_Check(obj)) {
            gil.Unlock();
            return ConvertListToXValue(obj);
        }
        else if (PyDict_Check(obj)) {
            gil.Unlock();
            return ConvertDictToXValue(obj);
        }
        else {
            gil.Unlock();
            X::Value valObj = CheckXlangObjectAndConvert(obj);
            if (!valObj.IsValid()){
                X::g_pXHost->PyObjToValue(obj, valObj);
            }
            return valObj;
        }
    }

    static PyObject* ConvertToPyObject(X::Value& value) {
        if (value.IsLong()) {
            MGil gil;
            return PyLong_FromLong((long)value);
        }
        else if (value.IsDouble()) {
            MGil gil;
            return PyFloat_FromDouble(value);
        }
        else if (value.IsString()) {
            MGil gil;
            return PyUnicode_FromString(value.ToString().c_str());
        }
        else if (value.IsList()) {
            return ConvertListToPyObject(value);
        }
        else if (value.IsDict()) {
            return ConvertDictToPyObject(value);
        }
        else if( value.IsObject()){
            auto* pXObj = value.GetObj();
            if (pXObj->GetType() == X::ObjType::PyProxyObject)
            {
				auto* pXPyObj = dynamic_cast<X::XPyObject*>(pXObj);
                if (pXPyObj)
                {
                    PyObject* pPyOb = Py_None;
                    pXPyObj->GetObj((void**)&pPyOb);
                    MGil gil;
                    Py_IncRef(pPyOb);
					return pPyOb;
                }
            }
            else
            {
                return CreateXlangObjectWrapper(value);
            }
        }
        else {
            MGil gil;
            PyObject* pOb = Py_None;
            Py_IncRef(pOb);
			return pOb;
		}
		return nullptr;
    }

    static bool ConvertArgs(PyObject* args, X::ARGS& x_args) {
        MGil gil;
        if (!PyTuple_Check(args)) {
            return false;
        }
        Py_ssize_t size = PyTuple_Size(args);
        x_args.resize((int)size);
		gil.Unlock();
        for (Py_ssize_t i = 0; i < size; ++i) {
			gil.Lock();
            PyObject* item = PyTuple_GetItem(args, i);
			gil.Unlock();
            x_args.push_back(ConvertToXValue(item));
        }
        return true;
    }

    static PyObject* ConvertArgs(X::ARGS& xArgs) {
        MGil gil;
        PyObject* pyArgs = PyTuple_New(xArgs.size());
        for (size_t i = 0; i < xArgs.size(); ++i) {
			gil.Unlock();
            PyObject* item = ConvertToPyObject(xArgs[i]);
			gil.Lock();
            PyTuple_SetItem(pyArgs, i, item);
        }
        return pyArgs;
    }

    static bool ConvertKwargs(PyObject* kwargs, X::KWARGS& x_kwArgs) {
        MGil gil;
        if (!PyDict_Check(kwargs)) {
            return false;
        }
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(kwargs, &pos, &key, &value)) {
            if (!PyUnicode_Check(key)) {
                return false;
            }
            const char* keyStr = PyUnicode_AsUTF8(key);
			gil.Unlock();
            x_kwArgs.Add(keyStr, ConvertToXValue(value));
			gil.Lock();
        }
        return true;
    }

    static PyObject* ConvertKwargs(X::KWARGS& xKwargs) {
        MGil gil;
        PyObject* pyKwargs = PyDict_New();
        for (auto& item : xKwargs) {
            PyObject* key = PyUnicode_FromString(item.key);
			gil.Unlock();
            PyObject* value = ConvertToPyObject(item.val);
			gil.Lock();
            PyDict_SetItem(pyKwargs, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
        }
        return pyKwargs;
    }

private:
    static X::Value ConvertListToXValue(PyObject* obj) {
        MGil gil;
        X::List list;
        Py_ssize_t size = PyList_Size(obj);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(obj, i);
			gil.Unlock();
            list += ConvertToXValue(item);
			gil.Lock();
        }
        return static_cast<X::Value>(list);
    }

    static X::Value ConvertDictToXValue(PyObject* obj) {
		MGil gil;
        X::Dict dict;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(obj, &pos, &key, &value)) {
			gil.Unlock();
            X::Value xKey = ConvertToXValue(key);
            X::Value xValue = ConvertToXValue(value);
            dict->Set(xKey, xValue);
			gil.Lock();
        }
        return static_cast<X::Value>(dict);
    }

    static PyObject* ConvertListToPyObject(X::Value& value) {
		MGil gil;   
        X::List list(value);
        PyObject* pyList = PyList_New(list.size());
        for (size_t i = 0; i < (size_t)list.size(); ++i) {
			gil.Unlock();
            PyObject* item = ConvertToPyObject(list[i]);
			gil.Lock();
            PyList_SetItem(pyList, i, item);
        }
        return pyList;
    }

    static PyObject* ConvertDictToPyObject(X::Value& value) {
		MGil gil;
        X::Dict dict(value);
        PyObject* pyDict = PyDict_New();
        dict->Enum([&gil,pyDict](X::Value& varKey, X::Value& val) {
			gil.Unlock();
            PyObject* p_key = ConvertToPyObject(varKey);
            PyObject* p_value = ConvertToPyObject(val);
			gil.Lock();
            PyDict_SetItem(pyDict, p_key, p_value);
            Py_DECREF(p_key);
            Py_DECREF(p_value);
            });
        return pyDict;
    }
};
