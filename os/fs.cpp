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

#include "fs.h"
#if (WIN32)
#include <Windows.h>
#endif
namespace fs = std::filesystem;

namespace X
{
	// Helper function: UTF-8 to UTF-16
	extern std::wstring UTF8ToWString(const std::string& utf8);
    extern std::filesystem::path U8Path(const std::string& utf8_str);
    bool FileSystem::Exists(std::string path) {
        try {
            return std::filesystem::exists(U8Path(path));
        }
        catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error checking existence: " << e.what() << std::endl;
            return false;
        }
    }

}