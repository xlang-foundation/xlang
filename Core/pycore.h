#ifndef __PYCORE_H__
#define __PYCORE_H__

namespace XPython {
typedef void* PyHandle;
struct String
{
	char* s;
	int size;
};

enum class KWIndex
{
	KW_S = 0,
	KW_E =34,
	Assign_Op_S = 35,
	Assign_Op_E = 47,
	Arithmetic_Op_S = 48,
	Arithmetic_Op_E = 54,
	Comparison_Op_S = 55,
	Comparison_Op_E = 60,
	Bitwise_Op_S = 61,
	Bitwise_Op_E = 66,
	Other_Op_S = 67,
	Other_Op_E = 76,
	Misc_Op_S = 77,
	Misc_Op_E = 79,
};
void PyInit(short* kwTree);
PyHandle PyLoad(char* code, int size);
bool PyRun(PyHandle h);
void PyClose(PyHandle);
}
#endif //__PYCORE_H__