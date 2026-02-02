#pragma once
#include "singleton.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <filesystem>

#include "PyFunc.h"

class PythonModulePathManager:
	public Singleton<PythonModulePathManager>
{
public:
    // Add path (refcount++)
    void AddPath(const std::filesystem::path& folder);

    // Remove path (refcount--, remove when reaching 0)
    void RemovePath(const std::filesystem::path& folder);

    // Optional helper: check if path is in sys.path
    bool HasPath(const std::filesystem::path& folder);

    PythonModulePathManager() = default;
    ~PythonModulePathManager() = default;
private:

    PythonModulePathManager(const PythonModulePathManager&) = delete;
    PythonModulePathManager& operator=(const PythonModulePathManager&) = delete;

    void AddToSysPath(const std::string& folderUtf8);
    void RemoveFromSysPath(const std::string& folderUtf8);

private:
    std::mutex m_lock;
    std::unordered_map<std::string, int> m_refCount; // UTF-8 folder �� count
};
