#pragma once
#include "PyFunc.h"
#include "xlang.h"
#include "xhost.h"

extern PyObject* CreateXlangObjectWrapper(X::Value& realObj);
extern X::Value CheckXlangObjectAndConvert(PyObject* obj);

class PyObjectXLangConverter {
public:
    static X::Value ConvertToXValue(PyObject* obj) {
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
            return ConvertListToXValue(obj);
        }
        else if (PyDict_Check(obj)) {
            return ConvertDictToXValue(obj);
        }
        else {

            X::Value valObj = CheckXlangObjectAndConvert(obj);
            if (!valObj.IsValid()){
                X::g_pXHost->PyObjToValue(obj, valObj);
            }
            return valObj;
        }
    }

    static PyObject* ConvertToPyObject(X::Value& value) {
        if (value.IsLong()) {
            return PyLong_FromLong((long)value);
        }
        else if (value.IsDouble()) {
            return PyFloat_FromDouble(value);
        }
        else if (value.IsString()) {
            return PyUnicode_FromString(value.ToString().c_str());
        }
        else if (value.IsList()) {
            return ConvertListToPyObject(value);
        }
        else if (value.IsDict()) {
            return ConvertDictToPyObject(value);
        }
        else if( value.IsObject()){
            return CreateXlangObjectWrapper(value);
        }
        else {
			return Py_None;
		}
    }

    static bool ConvertArgs(PyObject* args, X::ARGS& x_args) {
        if (!PyTuple_Check(args)) {
            return false;
        }
        Py_ssize_t size = PyTuple_Size(args);
        x_args.resize(size);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyTuple_GetItem(args, i);
            x_args.push_back(ConvertToXValue(item));
        }
        return true;
    }

    static PyObject* ConvertArgs(X::ARGS& xArgs) {
        PyObject* pyArgs = PyTuple_New(xArgs.size());
        for (size_t i = 0; i < xArgs.size(); ++i) {
            PyObject* item = ConvertToPyObject(xArgs[i]);
            PyTuple_SetItem(pyArgs, i, item);
        }
        return pyArgs;
    }

    static bool ConvertKwargs(PyObject* kwargs, X::KWARGS& x_kwArgs) {
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
            x_kwArgs.Add(keyStr, ConvertToXValue(value));
        }
        return true;
    }

    static PyObject* ConvertKwargs(X::KWARGS& xKwargs) {
        PyObject* pyKwargs = PyDict_New();
        for (auto& item : xKwargs) {
            PyObject* key = PyUnicode_FromString(item.key);
            PyObject* value = ConvertToPyObject(item.val);
            PyDict_SetItem(pyKwargs, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
        }
        return pyKwargs;
    }

private:
    static X::Value ConvertListToXValue(PyObject* obj) {
        X::List list;
        Py_ssize_t size = PyList_Size(obj);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(obj, i);
            list += ConvertToXValue(item);
        }
        return static_cast<X::Value>(list);
    }

    static X::Value ConvertDictToXValue(PyObject* obj) {
        X::Dict dict;
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(obj, &pos, &key, &value)) {
            X::Value xKey = ConvertToXValue(key);
            X::Value xValue = ConvertToXValue(value);
            dict->Set(xKey, xValue);
        }
        return static_cast<X::Value>(dict);
    }

    static PyObject* ConvertListToPyObject(X::Value& value) {
        X::List list(value);
        PyObject* pyList = PyList_New(list.size());
        for (size_t i = 0; i < list.size(); ++i) {
            PyObject* item = ConvertToPyObject(list[i]);
            PyList_SetItem(pyList, i, item);
        }
        return pyList;
    }

    static PyObject* ConvertDictToPyObject(X::Value& value) {
        X::Dict dict(value);
        PyObject* pyDict = PyDict_New();
        dict->Enum([pyDict](X::Value& varKey, X::Value& val) {
            PyObject* p_key = ConvertToPyObject(varKey);
            PyObject* p_value = ConvertToPyObject(val);
            PyDict_SetItem(pyDict, p_key, p_value);
            Py_DECREF(p_key);
            Py_DECREF(p_value);
            });
        return pyDict;
    }
};
