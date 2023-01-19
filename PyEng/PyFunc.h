#ifndef _PY_FUNC_H
#define _PY_FUNC_H

#include <string>

#ifdef _DEBUG
#undef _DEBUG
extern "C"
{
#include "Python.h"
}
#define _DEBUG
#else
extern "C"
{
#include "Python.h"
}
#endif

typedef struct _PyWrapFunc_
{
	PyObject_HEAD;
	void* pPyEngHost; //point to GrusPyEngHost
	void* pRealFunc;
	void* pContext;
} PyWrapFunc;

PyObject* CreateFuncWrapper(void* pPyEngHost,
	void* pRealFunc, void* pContext);

#endif // _PY_FUNC_H
