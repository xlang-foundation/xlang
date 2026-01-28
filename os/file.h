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

#include "xpackage.h"
#include "xlang.h"
#include <iostream>
#include <fstream>
#include <string>

namespace X {
    class File {
        std::ifstream m_stream;
        std::ofstream m_wstream;
        std::string m_fileName;
        bool m_IsBinary = true;
        bool m_IsWrite = false;
        bool m_IsOpen = false;

    public:
        BEGIN_PACKAGE(File)
            APISET().AddPropWithType<std::string>("name", &File::m_fileName);
            APISET().AddProp("closed", &File::get_closed);
            APISET().AddProp("mode", &File::get_mode);
            APISET().AddProp("size", &File::get_size);
            
            APISET().AddFunc<2>("seek", &File::seek);
            APISET().AddFunc<1>("read", &File::read);
            APISET().AddFunc<1>("write", &File::write);
            APISET().AddFunc<0>("close", &File::close);
            APISET().AddFunc<0>("flush", &File::flush);
            APISET().AddFunc<0>("tell", &File::tell);
            APISET().AddFunc<1>("readline", &File::readline);
            APISET().AddFunc<1>("readlines", &File::readlines);
            APISET().AddFunc<1>("truncate", &File::truncate);
            APISET().AddFunc<0>("readable", &File::readable);
            APISET().AddFunc<0>("writable", &File::writable);
            APISET().AddFunc<0>("seekable", &File::seekable);
        END_PACKAGE

        File() {}
        File(std::string fileName, std::string mode);
        ~File();

        bool seek(long long offset, int origin = 0);
        X::Value read(long long size = -1);
        long long write(X::Value p);
        bool close();
        bool flush();
        long long tell();
        X::Value readline(long long size = -1);
        X::Value readlines(long long hint = -1);
        long long truncate(long long size = -1);
        bool readable();
        bool writable();
        bool seekable();
        
        X::Value get_size();
        X::Value get_closed() { return !m_IsOpen; }
        X::Value get_mode(); 

    private: 
        std::string m_modeStr;
    };
}
