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

#include "struct.h"
#include "op.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
        // Initialize the size array
        const size_t XlangStruct::typeSizes[] = {
            0,
            sizeof(char),            // c_char
            sizeof(wchar_t),         // c_wchar
            sizeof(char),            // c_byte
            sizeof(unsigned char),   // c_ubyte
            sizeof(short),           // c_short
            sizeof(unsigned short),  // c_ushort
            sizeof(int),             // c_int
            sizeof(unsigned int),    // c_uint
            sizeof(long),            // c_long
            sizeof(unsigned long),   // c_ulong
            sizeof(long long),       // c_longlong
            sizeof(unsigned long long), // c_ulonglong
            sizeof(float),           // c_float
            sizeof(double),          // c_double
            sizeof(bool),            // c_bool
            sizeof(X::Value),        // X::Value
            sizeof(void*),            // c_void_p
            0                       //c_invalid
        };
        const std::string XlangStruct::typeNames[] = {
            "strut",
            "char", "wchar_t", "byte", "unsigned byte",
            "short", "unsigned short", "int", "unsigned int",
            "long", "unsigned long", "long long", "unsigned long long",
            "float", "double", "bool", "xvalue","void*"
        };
        static Obj_Func_Scope<1> _scope;
        void XlangStruct::Init()
        {
            _scope.Init();
            {
                auto f = [](X::XRuntime* rt, XObj* pThis, XObj* pContext,
                    ARGS& params,
                    KWARGS& kwParams,
                    X::Value& retValue)
                    {
                        return true;
                    };
                _scope.AddFunc("AddFields", "AddFields([{name,type,asPointer,bits}])", f);
            }
            _scope.Close();
        }
        void XlangStruct::cleanup()
        {
            _scope.Clean();
        }
        void XlangStruct::GetBaseScopes(std::vector<AST::Scope*>& bases)
        {
            bases.push_back(_scope.GetMyScope());
            bases.push_back(m_fieldScope.GetMyScope());
        }
        bool XlangStruct::Build()
        {
            int cnt = m_fields.size();
            m_fieldScope.InitWithNumber(cnt);
            for (int i = 0; i < cnt; i++)
            {
                auto& field = m_fields[i];
                XlangStructField* pField = new XlangStructField(field.name,i);
                X::Value objField(pField);
                m_fieldScope.AddObject(field.name.c_str(), objField);
            }
            m_fieldScope.Close();
            if (m_type == DataType::c_struct)
            {
                m_size = calculateStructureSize();
            }
            else
            {
                m_size = calculateUnionSize();
            }
            m_pData = new char[m_size];
            return true;
        }
	}
}