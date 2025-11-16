#include "PythonModulePathManager.h"
#include "PyGILState.h"
#include <filesystem>

namespace fs = std::filesystem;

// -----------------------------------------------------------
// Public: Check if sys.path already contains folder
// -----------------------------------------------------------
bool PythonModulePathManager::HasPath(const fs::path& folder)
{
    std::string utf8 = fs::absolute(folder).string();

    MGil gil; // ensure Python GIL
    PyObject* sysPath = PySys_GetObject("path"); // borrowed

    PyObject* pFolder = PyUnicode_FromString(utf8.c_str());
    int result = PySequence_Contains(sysPath, pFolder);
    Py_DECREF(pFolder);

    return result == 1;
}

// -----------------------------------------------------------
// Public: Add path, refcount++
// -----------------------------------------------------------
void PythonModulePathManager::AddPath(const fs::path& folder)
{
    std::string utf8 = fs::absolute(folder).string();

    std::lock_guard<std::mutex> guard(m_lock);

    auto it = m_refCount.find(utf8);
    if (it != m_refCount.end()) {
        // Already registered
        it->second++;
        return;
    }

    // First time this folder asked ¡ú insert into sys.path
    AddToSysPath(utf8);

    // Set initial refcount
    m_refCount[utf8] = 1;
}

// -----------------------------------------------------------
// Public: Remove path (refcount--, remove from sys.path when 0)
// -----------------------------------------------------------
void PythonModulePathManager::RemovePath(const fs::path& folder)
{
    std::string utf8 = fs::absolute(folder).string();

    std::lock_guard<std::mutex> guard(m_lock);

    auto it = m_refCount.find(utf8);
    if (it == m_refCount.end()) {
        return; // no such folder
    }

    it->second--;

    if (it->second > 0) {
        return; // still in use
    }

    // refcount reached 0 ¡ú remove from sys.path
    RemoveFromSysPath(utf8);
    m_refCount.erase(it);
}

// -----------------------------------------------------------
// Private: Actually insert folder into sys.path
// -----------------------------------------------------------
void PythonModulePathManager::AddToSysPath(const std::string& folderUtf8)
{
    MGil gil;

    PyObject* sysPath = PySys_GetObject("path"); // borrowed

    PyObject* pFolder = PyUnicode_FromString(folderUtf8.c_str());
    PyList_Insert(sysPath, 0, pFolder);  // Prepend for highest priority
    Py_DECREF(pFolder);
}

// -----------------------------------------------------------
// Private: Actually remove folder from sys.path
// -----------------------------------------------------------
void PythonModulePathManager::RemoveFromSysPath(const std::string& folderUtf8)
{
    MGil gil;

    PyObject* sysPath = PySys_GetObject("path");  // borrowed reference

    PyObject* pFolder = PyUnicode_FromString(folderUtf8.c_str());
    if (!pFolder)
        return;

    // Length of sys.path
    Py_ssize_t n = PyList_Size(sysPath);
    if (n < 0)
    {
        Py_DECREF(pFolder);
        return;
    }

    // Search for matching entry
    for (Py_ssize_t i = 0; i < n; ++i)
    {
        PyObject* item = PyList_GetItem(sysPath, i); // borrowed
        if (!item)
            continue;

        // Compare strings
        int eq = PyObject_RichCompareBool(item, pFolder, Py_EQ);
        if (eq == 1)
        {
            // Found ¡ú remove by index
            PySequence_DelItem(sysPath, i);
            break;
        }
    }

    Py_DECREF(pFolder);
}
