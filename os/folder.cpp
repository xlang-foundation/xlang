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

#include "fs.h"
#include "folder.h"

std::string WStringToUTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

namespace X
{

    Folder::Folder(const std::string& path)
    {
        std::string strPath = FileSystem::I().ConvertReletivePathToFullPath(path);
        std::filesystem::path fsPath = std::filesystem::path(strPath);
        // Remove trailing separators
        fsPath = fsPath.lexically_normal();
        folderPath = fsPath.make_preferred().string();
    }
}