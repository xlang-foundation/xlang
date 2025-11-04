#include "PyHelpFuncs.h"


static PyObject* g_inspect = nullptr;
static PyObject* g_getsourcelines = nullptr;
static PyObject* g_unwrap = nullptr;

static bool ensure_inspect_loaded() {
    if (g_getsourcelines && g_unwrap) return true;
    g_inspect = PyImport_ImportModule("inspect");   // new ref
    if (!g_inspect) return false;
    g_getsourcelines = PyObject_GetAttrString(g_inspect, "getsourcelines"); // new ref
    g_unwrap = PyObject_GetAttrString(g_inspect, "unwrap");         // new ref
    if (!g_getsourcelines || !g_unwrap) return false;
    return true;
}

std::string get_function_source(PyObject* fn) {
    // Requires GIL held.
    if (!ensure_inspect_loaded()) {
        PyErr_Clear();
        return std::string(); // ""
    }

    // Resolve wrappers (@functools.wraps, etc.)
    PyObject* real_fn = PyObject_CallFunctionObjArgs(g_unwrap, fn, NULL); // new ref
    if (!real_fn) { PyErr_Clear(); real_fn = fn; Py_INCREF(real_fn); }

    // (lines:list[str], first_lineno:int) = inspect.getsourcelines(real_fn)
    PyObject* res = PyObject_CallFunctionObjArgs(g_getsourcelines, real_fn, NULL); // new ref
    if (!res || !PyTuple_Check(res) || PyTuple_Size(res) < 2) {
        Py_XDECREF(res);
        Py_DECREF(real_fn);
        PyErr_Clear(); // e.g., builtins, C functions, interactive env
        return std::string();
    }

    PyObject* lines_list = PyTuple_GetItem(res, 0);  // borrowed
    if (!PyList_Check(lines_list)) {
        Py_DECREF(res);
        Py_DECREF(real_fn);
        return std::string();
    }

    std::string joined;
    Py_ssize_t n = PyList_Size(lines_list);
    joined.reserve(256);
    for (Py_ssize_t i = 0; i < n; ++i) {
        PyObject* s = PyList_GetItem(lines_list, i); // borrowed
        if (!PyUnicode_Check(s)) continue;
        PyObject* b = PyUnicode_AsUTF8String(s);     // new ref
        if (!b) { PyErr_Clear(); continue; }
        joined.append(PyBytes_AS_STRING(b), PyBytes_GET_SIZE(b));
        Py_DECREF(b);
    }

    Py_DECREF(res);
    Py_DECREF(real_fn);
    return joined; // full source block (including decorators), or "" if not available
}
