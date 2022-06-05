#ifndef __PYCORE_H__
#define __PYCORE_H__

namespace X {

typedef void* PyHandle;

PyHandle PyLoad(char* code, int size);
bool PyRun(PyHandle h);
void PyClose(PyHandle h);

}
#endif //__PYCORE_H__