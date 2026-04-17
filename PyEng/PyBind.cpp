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

#if (WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif
#include <string>

// Trick for win32 compile to avoid using pythonnn_d.lib
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

#include "XlangLoad.h"
#include "PyEngHostImpl.h"
#include <fstream>


extern PyMethodDef RootMethods[];

#ifdef PythonType2
PyMODINIT_FUNC initXlang(void)
{
    PyObject* m;
    Py_Initialize();
    PyImport_AddModule("xlang");
    Py_InitModule("xlang", RootMethods);
}
#else
static PyModuleDef XlangPackageTypeModule =
{
    PyModuleDef_HEAD_INIT,
    "xlang",
    "xlang Objects",
    -1,
    RootMethods, NULL, NULL, NULL, NULL
};

void cleanup()
{
    X::PyBind::UnloadXLangEngine();
}

static PyObject* cleanup_wrapper(PyObject* self, PyObject* args) {
    cleanup();
    Py_RETURN_NONE;
}

static PyMethodDef CleanupMethods[] = {
    {"cleanup", cleanup_wrapper, METH_NOARGS, "Cleanup function"},
    {NULL, NULL, 0, NULL}
};

static void register_cleanup() {
    PyObject* atexit = PyImport_ImportModule("atexit");
    if (atexit) {
        PyObject* cleanup_func = PyCFunction_New(&CleanupMethods[0], NULL);
        if (cleanup_func) {
            PyObject* result = PyObject_CallMethod(atexit, "register", "O", cleanup_func);
            Py_XDECREF(result);
            Py_DECREF(cleanup_func);
        }
        Py_DECREF(atexit);
    }
}

extern PyObject* CreateXlangObjectWrapper(X::Value& realObj);

PyMODINIT_FUNC PyInit_xlang(void)
{
    X::PyBind::ParamConfig paramCfg;
    paramCfg.appName = "xlang";

    std::string moduleDir;

#if (WIN32)
    HMODULE  hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)PyInit_xlang,
        &hModule);
    char path[MAX_PATH];
    GetModuleFileName(hModule, path, MAX_PATH);
    moduleDir = path;
    {
        auto pos = moduleDir.find_last_of("\\/");
        if (pos != std::string::npos)
            moduleDir = moduleDir.substr(0, pos);
    }
#else
    Dl_info dl_info;
    dladdr((void*)PyInit_xlang, &dl_info);
    moduleDir = dl_info.dli_fname;
    {
        auto pos = moduleDir.find_last_of('/');
        if (pos != std::string::npos)
            moduleDir = moduleDir.substr(0, pos);
    }
#endif

    // ---------------------------------------------------------
    // DEFAULT appPath (folder of xlang.pyd / xlang.so)
    // ---------------------------------------------------------
    paramCfg.appPath = moduleDir;

    // ---------------------------------------------------------
    // OVERRIDE if xlang_engine_path.txt exists
    // ---------------------------------------------------------
    std::string recordFile = moduleDir + "/xlang_engine_path.txt";
    std::ifstream ifs(recordFile);
    if (ifs.good()) {
        std::string recorded;
        std::getline(ifs, recorded);
        if (!recorded.empty()) {
            std::cout << "[xlang] Using engine path from record file:\n  "
                << recorded << "\n";
            paramCfg.appPath = recorded;
        }
    }
    ifs.close();

    // ---------------------------------------------------------
    // Load the engine
    // ---------------------------------------------------------
    PyObject* m = PyModule_Create(&XlangPackageTypeModule);
    std::string xlangSearchPath = paramCfg.appPath;
    X::PyBind::LoadXLangEngine(paramCfg, xlangSearchPath);

    X::g_pXHost->SetPyEngHost(&GrusPyEngHost::I());

    PyObject* gObj = PyDict_New();
    if (PyModule_AddObject(m, "g", gObj) < 0) {
        Py_DECREF(gObj);
    }
    X::Value rootObj;
    PyObject* xlangObj = CreateXlangObjectWrapper(rootObj);
    PyModule_AddObject(m, "xobj", xlangObj);

    // Inject all builtin functions into the root module
    X::Value builtinsValue;
    X::g_pXHost->GetBuiltins(builtinsValue);
    if (builtinsValue.IsList()) {
        X::List builtinList(builtinsValue);
        long long count = builtinList.Size();
        for (long long i = 0; i < count; ++i) {
            X::Value itemVals = builtinList[i];
            if (itemVals.IsList()) {
                X::List pair(itemVals);
                if (pair.Size() >= 2) {
                    X::Value nameVal = pair[0];
                    X::Value funcVal = pair[1];
                    std::string funcName = nameVal.ToString();
                    PyObject* pyFunc = CreateXlangObjectWrapper(funcVal);
                    PyModule_AddObject(m, funcName.c_str(), pyFunc);
                }
            }
        }
    }

    register_cleanup();
    return m;
}

#endif
