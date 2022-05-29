#ifndef __PYCORE_H__
#define __PYCORE_H__

namespace XPython {

typedef void* PyHandle;

void PyInit(short* kwTree);
PyHandle PyLoad(char* code, int size);
bool PyRun(PyHandle h);
void PyClose(PyHandle);

}
#endif //__PYCORE_H__