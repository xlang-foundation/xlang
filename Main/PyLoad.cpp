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

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#include <memory>
#include <array>
#endif

namespace fs = std::filesystem;

// --- Run Python command and capture output ---
std::string run_python_cmd(const std::string& code) {
    std::string result;

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return {};
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmd = "python -c \"" + code + "\"";

    if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead); CloseHandle(hWrite);
        return {};
    }

    CloseHandle(hWrite);
    char buffer[256];
    DWORD read;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
        buffer[read] = 0;
        result += buffer;
    }
    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    std::array<char, 256> buffer{};
    std::string cmd = "python3 -c '" + code + "' 2>/dev/null";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return {};
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
#endif

    if (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.erase(result.find_last_not_of("\r\n") + 1);
    return result;
}

// --- Find ABI-matching pyeng file in folder ---
std::string find_pyeng_module(const std::string& folder) {
    std::string ext_suffix = run_python_cmd("import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))");
    if (ext_suffix.empty()) return {};

#ifdef _WIN32
    // Build expected .dll (not .pyd)
    fs::path expected = fs::path(folder) / ("pyeng" + ext_suffix.substr(0, ext_suffix.rfind(".")) + ".dll");
    if (fs::exists(expected)) return expected.string();

    // Fallback: scan for pyeng*.dll
    for (auto& p : fs::directory_iterator(folder)) {
        if (p.path().filename().string().rfind("pyeng", 0) == 0 &&
            p.path().extension() == ".dll") {
            return p.path().string();
        }
    }
#else
    fs::path expected = fs::path(folder) / ("pyeng" + ext_suffix);
    if (fs::exists(expected)) return expected.string();
#endif
    return {};
}

// --- Copy module into site-packages as xlang ---
bool install_into_sitepackages(const std::string& modulePath) {
    std::string site = run_python_cmd(
        "import sysconfig; print(sysconfig.get_paths()['purelib'])"
    );
    if (site.empty()) {
        std::cerr << "Could not locate site-packages\n";
        return false;
    }

    fs::path src(modulePath);
    fs::path dstDir(site);
    fs::path dst;

#ifdef _WIN32
    dst = dstDir / "xlang.pyd"; // copy DLL as .pyd
#else
    dst = dstDir / "xlang.so";
#endif

    try {
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
        std::cout << "Installed plugin into: " << dst << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Copy failed: " << e.what() << "\n";
        return false;
    }
}

// helper: convert file_time_type → time_t
inline std::time_t to_time_t(std::filesystem::file_time_type ftime) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now()
        + system_clock::now()
    );
    return system_clock::to_time_t(sctp);
}

// --- Resolve path with cache file (extended) ---
std::string resolve_pyeng_path(const std::string& folder) {
    fs::path cacheFile = fs::path(folder) / "pyeng.cache.txt";

    std::string module;
    bool copied = false;
    std::time_t cachedTime = 0;
    uintmax_t cachedSize = 0;

    if (fs::exists(cacheFile)) {
        std::ifstream fin(cacheFile);
        fin >> module >> copied >> cachedTime >> cachedSize;
        fin.close();

        if (!module.empty() && fs::exists(module)) {
            auto ftime = fs::last_write_time(module);
            std::time_t modTime = to_time_t(ftime);
            uintmax_t size = fs::file_size(module);

            if (modTime == cachedTime && size == cachedSize) {
                std::cout << "Loaded cached path: " << module
                    << " (already copied=" << copied << ")\n";
                return module;
            }
            else {
                std::cout << "Module changed, will re-copy.\n";
            }
        }
    }

    // Not cached or changed → detect
    module = find_pyeng_module(folder);
    if (module.empty()) {
        std::cerr << "No pyeng module found in: " << folder << "\n";
        return {};
    }

    copied = install_into_sitepackages(module);

    // Collect new metadata
    auto ftime = fs::last_write_time(module);
    std::time_t modTime = to_time_t(ftime);
    uintmax_t size = fs::file_size(module);

    // Save cache with metadata
    std::ofstream fout(cacheFile);
    fout << module << " " << copied << " " << modTime << " " << size;
    fout.close();

    return module;
}


