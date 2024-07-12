#include "struct.h"
#include "op.h"
#include "obj_func_scope.h"

namespace X
{
	namespace Data
	{
        // Initialize the size array
        const size_t XlangStruct::typeSizes[] = {
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
            sizeof(void*),            // c_void_p
            0                       //c_invalid
        };
        const std::string XlangStruct::typeNames[] = {
            "char", "wchar_t", "byte", "unsigned byte",
            "short", "unsigned short", "int", "unsigned int",
            "long", "unsigned long", "long long", "unsigned long long",
            "float", "double", "bool", "void*"
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
	}
}