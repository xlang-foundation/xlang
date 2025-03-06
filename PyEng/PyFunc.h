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
