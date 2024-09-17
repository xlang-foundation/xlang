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

#if (WIN32)
    HMODULE  hModule = NULL;
    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)PyInit_xlang,
        &hModule);
    char path[MAX_PATH];
    GetModuleFileName(hModule, path, MAX_PATH);
    std::string strPath(path);
    auto pos = strPath.rfind("\\");
    if (pos != std::string::npos)
    {
        strPath = strPath.substr(0, pos + 1);
    }
    strPath += "\\xlang";
    paramCfg.appPath = strPath;
#else
    Dl_info dl_info;
    dladdr((void*)PyInit_xlang, &dl_info);
    std::string strPath = dl_info.dli_fname;
    auto pos = strPath.rfind("/");
    if (pos != std::string::npos)
    {
        strPath = strPath.substr(0, pos + 1);
    }
    strPath += "/xlang";
    paramCfg.appPath = strPath;
#endif
    PyObject* m = PyModule_Create(&XlangPackageTypeModule);
    std::string xlangSearchPath = paramCfg.appPath;
    X::PyBind::LoadXLangEngine(paramCfg, xlangSearchPath);

    X::g_pXHost->SetPyEngHost(&GrusPyEngHost::I());

    // JITManager::I().SetThisModule(m);
    PyObject* gObj = PyDict_New();
    if (PyModule_AddObject(m, "g", gObj) < 0)
    {
        Py_DECREF(gObj);
    }
    X::Value rootObj;
    PyObject* xlangObj = CreateXlangObjectWrapper(rootObj);
    PyModule_AddObject(m, "xobj", xlangObj);
    // Register the cleanup function
    register_cleanup();

    return m;
}
#endif
