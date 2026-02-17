/*
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

#include "xlang.h"
#include "xhost.h"
#include <cstring>
#include "utility.h"

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

#include "PyBinarySerializer.h"
#include "PyObjectXLangConverter.h"

extern PyObject* CreateXlangObjectWrapper(X::Value& realObj);

static PyObject*
Xlang_main(PyObject* self, PyObject* args, PyObject* kwargs)
{
	printf("inside Xlang_main\r\n");
	return PyLong_FromLong(0);
}

// Helper function to get a Python function by name from __main__ globals
static X::Value GetPythonFunctionByName(const std::string& funcName)
{
	// Get the __main__ module
	PyObject* mainModule = PyImport_AddModule("__main__");
	if (!mainModule) {
		return X::Value();
	}

	// Get the globals dictionary from __main__
	PyObject* globalsDict = PyModule_GetDict(mainModule);
	if (!globalsDict) {
		return X::Value();
	}

	// Get the function object by name
	PyObject* funcObj = PyDict_GetItemString(globalsDict, funcName.c_str());
	if (!funcObj) {
		return X::Value();
	}

	// Check if it's callable
	if (!PyCallable_Check(funcObj)) {
		return X::Value();
	}

	// Convert PyObject to X::Value
	X::Value funcValue = PyObjectXLangConverter::ConvertToXValue(funcObj);
	return funcValue;
}

//#include <Windows.h>
static PyObject* MainEventLoop(PyObject* self, PyObject* args, PyObject* kwargs)
{
    PyObject* eventSource = nullptr;
    if (!PyArg_ParseTuple(args, "O", &eventSource)) {
        Py_RETURN_FALSE;
    }

    if (!eventSource) {
        PyErr_SetString(PyExc_TypeError, "eventSource cannot be None");
        Py_RETURN_FALSE;
    }
    // Check if __file__ is passed in kwargs first
    const char* callingFile = nullptr;
    if (kwargs) {
        PyObject* fileKwarg = PyDict_GetItemString(kwargs, "__file__");
        if (fileKwarg && PyUnicode_Check(fileKwarg)) {
            callingFile = PyUnicode_AsUTF8(fileKwarg);
        }
    }
    // Get __file__ from the calling frame
    if (!callingFile) {
        PyFrameObject* frame = PyEval_GetFrame();
        if (frame) {
#if PY_VERSION_HEX >= 0x030B0000  // Python 3.11+
            PyObject* globals = PyFrame_GetGlobals(frame);
            if (globals) {
                PyObject* fileObj = PyDict_GetItemString(globals, "__file__");
                if (fileObj && PyUnicode_Check(fileObj)) {
                    callingFile = PyUnicode_AsUTF8(fileObj);
                }
                Py_DECREF(globals);
            }
#else  // Python 3.9, 3.10
            PyObject* globals = NULL;
#if PY_VERSION_HEX >= 0x03090000
            globals = PyModule_GetDict(PyImport_AddModule("__main__"));
#else
            if (frame != NULL) {
                globals = frame->f_globals;
                Py_INCREF(globals);
            }
#endif
#endif
        }
    }
    // Convert eventSource to X::Value
    X::Value varEventSource = PyObjectXLangConverter::ConvertToXValue(eventSource);

    // Get sys.argv and convert to X::Value
    PyObject* sysModule = PyImport_ImportModule("sys");
    PyObject* sysArgv = nullptr;
    if (sysModule) {
        sysArgv = PyObject_GetAttrString(sysModule, "argv");
        Py_DECREF(sysModule);
    }
    if (!sysArgv) {
        PyErr_Clear();
        sysArgv = PyList_New(0);
    }
    X::Value varSysArgs = PyObjectXLangConverter::ConvertToXValue(sysArgv);
    Py_DECREF(sysArgv);
	X::Value varFileName(callingFile ? callingFile : "");
    X::ARGS varArgs(1);
	varArgs.push_back(varSysArgs);

	X::KWARGS varKwargs;
	varKwargs.Add("__file__", varFileName);
	// Add current process ID for PullEvents
    unsigned long processId = GetPID();
	varKwargs.Add("__pid__", X::Value((long long)processId));
    X::Value varPullEvents = varEventSource["PullEvents"];
    X::Value varSetResults = varEventSource["PythonProcessSetResults"];
    bool running = true;
    while (running) {
        // Allow Python to handle signals and other threads
        if (PyErr_CheckSignals() != 0) {
            // Ctrl+C or other signal received
            break;
        }

        X::Value eventsList = varPullEvents.ObjCall(varArgs,varKwargs);

        if (eventsList.IsList()) {
            X::List outerList(eventsList);
            long long listSize = outerList.Size();

            // Iterate through each [func, param] pair
            for (long long i = 0; i < listSize; i++) {
                X::Value itemVal = outerList[i];

                if (itemVal.IsList()) {
                    X::List eventPair(itemVal);
                    if (eventPair.Size() >= 2) {
                        X::Value funcVal = eventPair[0];
                        if (funcVal.isString()) {
                            // Check if it is Stop, or Exit or Bye
                            std::string strSigal = funcVal.ToString();
                            if (strSigal == "Stop" || strSigal == "Exit" || strSigal == "Bye") {
                                running = false;
                                break;
                            }
                            else {
                                // It's a function name, get the function from Python globals
                                funcVal = GetPythonFunctionByName(strSigal);
                                if (!funcVal.IsValid()) {
                                    // Function not found, skip this event
                                    continue;
                                }
                            }
                        }
                        X::Value paramVal = eventPair[1];
                        X::Value varCallRetValue;
                        if (paramVal.IsList())
                        {
                            X::List paramList(paramVal);
							int argsNum = (int)paramList.Size();
                            if (argsNum >= 1)
                            {
                                X::ARGS args(argsNum);
                                for (long long j = 0; j < paramList.Size(); j++)
                                {
                                    args.push_back(paramList[j]);
                                }
                                varCallRetValue = funcVal.ObjCall(args);
                            }
                            else
                            {
                                X::ARGS args(1);
								args.push_back(paramVal);
                                varCallRetValue = funcVal.ObjCall(args);
                            }
                        }
                        else
                        {
                            X::ARGS args(1);
                            args.push_back(paramVal);
                            varCallRetValue = funcVal.ObjCall(args);
                        }
                        if (varSetResults.IsValid() && eventPair.Size() >= 3)
                        {
                            X::KWARGS kwargs0;
                            kwargs0.Add("__file__", varFileName);
                            kwargs0.Add("__pid__", X::Value((long long)processId));
                            kwargs0.Add("__callId__", eventPair[2]);
                            X::ARGS args0(1);
                            args0.push_back(varCallRetValue);
                            varSetResults.ObjCall(args0, kwargs0);
                        }
                    }
                }
            }
        }
    }

    Py_RETURN_TRUE;
}

static PyObject*
Xlang_import(PyObject* self, PyObject* args, PyObject* kwargs)
{
	const char* moduleName = nullptr;
	const char* from = nullptr;
	const char* thru = nullptr;
	static char* kwlist[] = { "moduleName", "fromPath", "thru", nullptr };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ss", kwlist, &moduleName, &from, &thru))
	{
		return nullptr;
	}

	auto* rt = X::g_pXHost->GetCurrentRuntime();
	X::Value obj;
	bool bOK = false;
	Py_BEGIN_ALLOW_THREADS
		bOK = X::g_pXHost->Import(rt, moduleName, from, thru, obj);
	Py_END_ALLOW_THREADS
	if (!bOK)
	{
		PyErr_SetString(PyExc_RuntimeError, "Import failed");
		return nullptr;
	}
	return CreateXlangObjectWrapper(obj);
}


static PyObject*
Xlang_Function(PyObject* self, PyObject* args, PyObject* kwargs)
{
	Py_ssize_t size = PyTuple_Size(args);
	for (Py_ssize_t i = 0; i < size; ++i) {
		PyObject* item = PyTuple_GetItem(args, i);
		item = item;
	}
	return PyLong_FromLong(0);
}
static PyObject*
Xlang_Class(PyObject* self, PyObject* args, PyObject* kwargs)
{
	return PyLong_FromLong(0);
}


// NOTE: adjust these helpers if your X::Bin API differs.
static bool ExtractBytesFromXLangBin(const X::Value& v, const char*& data, size_t& size) {
    // Expect v to be an X::Bin. Pattern follows how Dict is wrapped in PyObjectXLangConverter.h:
    //     X::Dict dict(value); dict->Enum(...)
    X::Bin bin(v);                  // construct a typed view over v
    if (!bin.IsValid()) return false;
    // These method names are typical; if your X::Bin differs, swap to your API:
    data = reinterpret_cast<const char*>(bin->Data());   // pointer to raw bytes
    size = static_cast<size_t>(bin->Size());             // byte length
    return (data != nullptr);
}

static PyObject*
Xlang_Dump(PyObject* self, PyObject* args, PyObject* kwargs)
{
    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "Xlang_Dump expects 1 argument");
        return nullptr;
    }
    PyObject* item = PyTuple_GetItem(args, 0); // borrowed

    std::string outBytes;
	PyBinarySerializer pb;
    if (!pb.Dump(item, outBytes, PySerOptions())) {
        // PyBinarySerializer sets a Python exception on failure.
        return nullptr;
    }
	X::Bin binObj((unsigned long long)outBytes.size(), true);
	std::memcpy(binObj->Data(), outBytes.data(), outBytes.size());
    return CreateXlangObjectWrapper(binObj);
}

static PyObject*
Xlang_Load(PyObject* self, PyObject* args, PyObject* kwargs)
{
    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "Xlang_Load expects 1 argument");
        return nullptr;
    }

    PyObject* src = PyTuple_GetItem(args, 0); // borrowed

    // Path A: bytes-like directly from Python.
    const char* data_ptr = nullptr;
    size_t      data_len = 0;

    if (PyBytes_Check(src)) {
        char* p = nullptr; Py_ssize_t n = 0;
        if (PyBytes_AsStringAndSize(src, &p, &n) < 0) return nullptr;
        data_ptr = p;
        data_len = static_cast<size_t>(n);
    }
    else if (PyByteArray_Check(src)) {
        data_ptr = PyByteArray_AsString(src);
        if (!data_ptr) return nullptr;
        data_len = static_cast<size_t>(PyByteArray_Size(src));
    }
    else {
		X::Value xval = CheckXlangObjectAndConvert(src);
        if (!ExtractBytesFromXLangBin(xval, data_ptr, data_len)) {
            PyErr_SetString(PyExc_TypeError, "Xlang_Load expects an X::Bin or bytes-like object");
            return nullptr;
        }
    }

    // Now decode the binary blob into a Python object (NEW ref or NULL with exception set).
	PyBinarySerializer pb;
	PyObject* restored = pb.Load(data_ptr, data_len, PyDeserOptions());
    return restored;
}


PyMethodDef RootMethods[] =
{
	{	"main",
		(PyCFunction)Xlang_main,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax xlang.main(*args,**kwargs)"
	},
    {	"MainEventLoop",
        (PyCFunction)MainEventLoop,
        METH_VARARGS | METH_KEYWORDS,
        "Syntax xlang.MainEventLoop(*args,**kwargs)"
    },
	{	"func", 
		(PyCFunction)Xlang_Function,
		METH_VARARGS|METH_KEYWORDS,
		"Syntax: wrapper = xlang.func(*args,**kwargs)" 
	},
	{	"importModule",
		(PyCFunction)Xlang_import,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: wrapper = xlang.importModule(*args,**kwargs)"
	},
	{	"Dump",
		(PyCFunction)Xlang_Dump,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: bytes = xlang.dump(*args,**kwargs)"
	},
	{	"Load",
		(PyCFunction)Xlang_Load,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: object = xlang.load(*args,**kwargs)"
	},
	{	"object",
		(PyCFunction)Xlang_Class,
		METH_VARARGS | METH_KEYWORDS,
		"Syntax: wrapper = xlang.object(*args,**kwargs)"
	},
	{ NULL, NULL, 0, NULL }
};

