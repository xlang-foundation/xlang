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

#include "folder.h"
#include <iostream>
#include <filesystem> // For std::filesystem operations
#include <stdexcept>  // For std::runtime_error
#if (WIN32)
#include <windows.h> // For Windows API functions like MultiByteToWideChar
#endif

namespace X {

    // Constructor
    Folder::Folder(const std::string& path) {
        std::string norm_path = path;
        if (!norm_path.empty()) {
            std::filesystem::path fs_p = std::filesystem::u8path(norm_path);
            fs_p.make_preferred();
			auto u8str = fs_p.lexically_normal().u8string();
            norm_path = std::string(u8str.begin(), u8str.end());
        }
#if (WIN32)
        folderPath = norm_path; // If empty in Windows, enumerate all drives.
#elif (__APPLE__)
        if (norm_path.empty()) {
            // Retrieve the user's home directory using getpwuid.
            const char* home = std::getenv("HOME");
            folderPath = home; // Set to the user's home directory.
        }
        else {
            folderPath = norm_path;
        }
#else
        if (norm_path.empty()) {
            folderPath = "/"; // On other systems, treat empty as the root directory.
        }
        else {
            folderPath = norm_path;
        }
#endif
    }


    // Helper function: UTF-8 to UTF-16
    std::wstring UTF8ToWString(const std::string& utf8) {
#if (WIN32)
        if (utf8.empty()) return {};
        int wstrLength = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (wstrLength <= 0) throw std::runtime_error("MultiByteToWideChar failed.");
        std::wstring wstr(wstrLength - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wstrLength);
        return wstr;
#else
        throw std::runtime_error("UTF8ToWString is Windows-specific.");
#endif
    }

    // Helper function: UTF-16 to UTF-8
    std::string WStringToUTF8(const std::wstring& wstr) {
#if (WIN32)
        if (wstr.empty()) return {};
        int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Length <= 0) throw std::runtime_error("WideCharToMultiByte failed.");
        std::string utf8(utf8Length - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], utf8Length, nullptr, nullptr);
        return utf8;
#else
        throw std::runtime_error("WStringToUTF8 is Windows-specific.");
#endif
    }

    // BuildPath
    std::string Folder::BuildPath(const std::string& subPath) {
        try {
            std::filesystem::path fullPath = std::filesystem::u8path(folderPath);
            fullPath /= std::filesystem::u8path(subPath);
            auto u8str = fullPath.lexically_normal().u8string();
            return std::string(u8str.begin(), u8str.end());
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error building path: " << e.what() << std::endl;
            return {};
        }
    }

    // List
    X::List Folder::List() {
        X::List resultList;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(folderPath))) {
                auto u8str = entry.path().filename().u8string();
                resultList += std::string(u8str.begin(), u8str.end());
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error while accessing folder: " << e.what() << std::endl;
        }
        return resultList;
    }

    // Scan
    X::List Folder::Scan() {
        X::List resultList;
        if (folderPath.empty()) {
#if (WIN32)
            char driveLetter = 'A';
            DWORD driveMask = GetLogicalDrives();
            if (driveMask == 0) return resultList;
            while (driveMask) {
                if (driveMask & 1) {
                    std::string drivePath = std::string(1, driveLetter) + ":";
                    X::Dict driveInfo;
                    driveInfo->Set("Name", drivePath);
                    driveInfo->Set("Path", drivePath+"\\");
                    driveInfo->Set("IsDirectory", "true");
                    driveInfo->Set("Size", "N/A");
                    driveInfo->Set("LastModified", "N/A");
                    resultList += driveInfo;
                }
                driveLetter++;
                driveMask >>= 1;
            }
#endif
        }

        try {
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::u8path(folderPath))) {
                X::Dict fileInfo;

                // Get the file name as a UTF-8 string
                auto u8str = entry.path().filename().u8string();
                fileInfo->Set("Name", std::string(u8str.begin(), u8str.end()));

                // Set the full path as a UTF-8 string
                auto fullPathStr = entry.path().u8string();
                fileInfo->Set("Path", std::string(fullPathStr.begin(), fullPathStr.end()));

                // Check if it's a directory
                fileInfo->Set("IsDirectory", entry.is_directory() ? "true" : "false");

                // If it's a file, set its size
                if (!entry.is_directory()) {
                    fileInfo->Set("Size", std::to_string(std::filesystem::file_size(entry.path())));
                }

                // Add the fileInfo to the result list
                resultList += fileInfo;
            }

        }
        catch (const std::filesystem::filesystem_error& e) {
            //std::cerr << "Error scanning folder: " << e.what() << std::endl;
        }

        return resultList;
    }

    // CopyFolder
    bool Folder::CopyFolder(const std::string& targetPath) {
        try {
            std::filesystem::copy(std::filesystem::u8path(folderPath),
                std::filesystem::u8path(targetPath),
                std::filesystem::copy_options::recursive);
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error copying folder: " << e.what() << std::endl;
            return false;
        }
    }

    // CopyFile
    bool Folder::CopyFileImpl(const std::string& filePath, const std::string& targetPath) {
        try {
            std::filesystem::copy(std::filesystem::u8path(filePath),
                std::filesystem::u8path(targetPath));
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error copying file: " << e.what() << std::endl;
            return false;
        }
    }

    // CreateFolder
    bool Folder::CreateFolder(const std::string& path) {
        try {
            std::filesystem::create_directories(std::filesystem::u8path(path));
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating folder: " << e.what() << std::endl;
            return false;
        }
    }

    // RemoveFolder
    bool Folder::RemoveFolder(const std::string& path, bool recursive) {
        try {
            if (recursive) {
                std::filesystem::remove_all(std::filesystem::u8path(path));
            }
            else {
                std::filesystem::remove(std::filesystem::u8path(path));
            }
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error removing folder: " << e.what() << std::endl;
            return false;
        }
    }

    // Rename
    bool Folder::Rename(const std::string& srcPath, const std::string& targetPath) {
        try {
            std::filesystem::rename(std::filesystem::u8path(srcPath),
                std::filesystem::u8path(targetPath));
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error renaming path: " << e.what() << std::endl;
            return false;
        }
    }

    // DeleteFile
    bool Folder::DeleteFileImpl(const std::string& filePath) {
        try {
            std::filesystem::remove(std::filesystem::u8path(filePath));
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error deleting file: " << e.what() << std::endl;
            return false;
        }
    }

    // MoveFolder
    bool Folder::MoveFolder(const std::string& targetPath) {
        try {
            std::filesystem::rename(std::filesystem::u8path(folderPath),
                std::filesystem::u8path(targetPath));
            folderPath = targetPath;
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error moving folder: " << e.what() << std::endl;
            return false;
        }
    }

    // ParentPath
    std::string Folder::ParentPath() const {
        try {
            auto u8str = std::filesystem::u8path(folderPath).parent_path().u8string();
            return std::string(u8str.begin(), u8str.end());
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error getting parent path: " << e.what() << std::endl;
            return {};
        }
    }

    // RelativePath
    std::string Folder::RelativePath(const std::string& basePath) const {
        try {
            auto u8str = std::filesystem::relative(std::filesystem::u8path(folderPath),
                std::filesystem::u8path(basePath))
                .u8string();
            return std::string(u8str.begin(), u8str.end());
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error calculating relative path: " << e.what() << std::endl;
            return {};
        }
    }

    // IsAbsolutePath
    bool Folder::IsAbsolutePath(const std::string& path) const {
#if (WIN32)
        return path.size() > 2 && path[1] == ':' &&
            (path[2] == '\\' || path[2] == '/');
#else
        return !path.empty() && path[0] == '/';
#endif
    }
    // Exists: Check if the folder path exists
    bool Folder::Exists() {
        try {
            return std::filesystem::exists(std::filesystem::u8path(folderPath));
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error checking existence: " << e.what() << std::endl;
            return false;
        }
    }
} // namespace X
