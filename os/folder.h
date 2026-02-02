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

#pragma once

#include <string>
#include <filesystem>
#include "xpackage.h"
#include "xlang.h"

namespace X {

    class Folder {
    public:
        BEGIN_PACKAGE(Folder)
            APISET().AddPropWithType<std::string>("Path", &Folder::folderPath);
            APISET().AddFunc<1>("BuildPath", &Folder::BuildPath);
            APISET().AddFunc<0>("Scan", &Folder::Scan);
            APISET().AddFunc<0>("List", &Folder::List);
            APISET().AddFunc<1>("CopyFolder", &Folder::CopyFolder);
            APISET().AddFunc<2>("CopyFile", &Folder::CopyFileImpl);
            APISET().AddFunc<1>("CreateFolder", &Folder::CreateFolder);
            APISET().AddFunc<2>("RemoveFolder", &Folder::RemoveFolder);
            APISET().AddFunc<2>("Rename", &Folder::Rename);
            APISET().AddFunc<1>("DeleteFile", &Folder::DeleteFileImpl);
            APISET().AddFunc<1>("MoveFolder", &Folder::MoveFolder);
            APISET().AddFunc<0>("ParentPath", &Folder::ParentPath);
            APISET().AddFunc<1>("RelativePath", &Folder::RelativePath);
            APISET().AddFunc<1>("IsAbsolutePath", &Folder::IsAbsolutePath);
            APISET().AddFunc<0>("Exists", &Folder::Exists);

            // New Python-like APIs and os module equivalence
            APISET().AddFunc<1>("glob", &Folder::glob);
            APISET().AddFunc<1>("walk", &Folder::walk);
            APISET().AddFunc<1>("is_under", &Folder::is_under);
            
            // os module aliases/wrappers
            APISET().AddFunc<1>("mkdir", &Folder::CreateFolder); // alias
            APISET().AddFunc<1>("makedirs", &Folder::makedirs);
            APISET().AddFunc<1>("remove", &Folder::DeleteFileImpl); // alias
            APISET().AddFunc<1>("rmdir", &Folder::rmdir);
            APISET().AddFunc<1>("removedirs", &Folder::removedirs);
            APISET().AddFunc<2>("rename", &Folder::Rename); // alias
            APISET().AddFunc<1>("stat", &Folder::stat);
            APISET().AddFunc<2>("access", &Folder::access);
            APISET().AddFunc<2>("chmod", &Folder::chmod);
            APISET().AddFunc<0>("getcwd", &Folder::getcwd);
            APISET().AddFunc<1>("chdir", &Folder::chdir);
            
        END_PACKAGE

        explicit Folder(const std::string& path);

        std::string BuildPath(const std::string& subPath);
        X::List List();
        X::List Scan();
        bool CopyFolder(const std::string& targetPath);
        bool CopyFileImpl(const std::string& filePath, const std::string& targetPath);
        bool CreateFolder(const std::string& path);
        bool RemoveFolder(const std::string& path, bool recursive = false);
        bool Rename(const std::string& srcPath, const std::string& targetPath);
        bool DeleteFileImpl(const std::string& filePath);
        bool MoveFolder(const std::string& targetPath);
        std::string ParentPath() const;
        std::string RelativePath(const std::string& basePath) const;
        bool IsAbsolutePath(const std::string& path) const;
        bool Exists();
        
        // New APIs
        X::List glob(const std::string& pattern);
        X::List walk(bool topdown = true); // Simplified walk
        bool is_under(const std::string& path);
        
        bool makedirs(const std::string& path);
        bool rmdir(const std::string& path);
        bool removedirs(const std::string& path);
        X::Value stat(const std::string& path);
        bool access(const std::string& path, int mode = 0);
        bool chmod(const std::string& path, int mode);
        std::string getcwd();
        bool chdir(const std::string& path);

    private:
        std::string folderPath;
    };
} // namespace X
