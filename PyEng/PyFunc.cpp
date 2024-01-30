#include "PyFunc.h"
#include "PyEngHostImpl.h"

static PyObject*
WrapFunc_new(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
	PyWrapFunc* self = (PyWrapFunc*)type->tp_alloc(type, 0);
	self->pRealFunc = nullptr;
	self->pContext = nullptr;
	return (PyObject*)self;
}
static int
WrapFunc_init(PyWrapFunc* self, PyObject* args, PyObject* kwds)
{
	return 0;
}
static void WrapFunc_dealloc(PyWrapFunc* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject WrapFuncType = {
	PyVarObject_HEAD_INIT(&PyType_Type,sizeof(PyType_Type))
	"XlangPythonWrapFunc",             /*tp_name*/
	sizeof(PyWrapFunc),             /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)WrapFunc_dealloc, /*tp_dealloc*/
	0,/*tp_vectorcall_offset*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_as_async*/

	0,                         /*tp_repr*/

	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/

	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/

	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Xlang Python WrapFunc objects",           /* tp_doc */

	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
	0,             /* tp_methods */
	0,             /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)WrapFunc_init,      /* tp_init */
	0,                         /* tp_alloc */
	WrapFunc_new,                 /* tp_new */
	0,//tp_free
	0,//tp_is_gc
	0,//tp_bases
	0,//tp_mro
	0,//tp_cache
	0,//tp_subclasses
	0,//tp_weaklist
	0,//tp_del
	0,//tp_version_tag
	0,//tp_finalize
	0,//tp_vectorcall
};

static bool __WrapFuncType_Prepared = false;
void PrepareWrapFuncType()
{
	PyType_Ready(&WrapFuncType);
	__WrapFuncType_Prepared = true;
}

PyWrapFunc* NewPyWrapFunc(void* pPyEngHost,
	void* pRealFunc,void* pContext)
{
	if (!__WrapFuncType_Prepared)
	{
		PrepareWrapFuncType();
	}
	auto self = (PyWrapFunc*)WrapFuncType.tp_alloc(&WrapFuncType, 0);
	self->pPyEngHost = pPyEngHost;
	self->pRealFunc = pRealFunc;
	self->pContext = pContext;
	return self;
}
void DestroyPyWrapFunc(PyWrapFunc* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

//real function call from python side
static PyObject*
XlangPythonWrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	PyWrapFunc* pWrapFunc = (PyWrapFunc*)self;
	GrusPyEngHost* pPyEngHost = (GrusPyEngHost*)pWrapFunc->pPyEngHost;
	Xlang_CallFunc callFunc = pPyEngHost->GetXlangCallFunc();
	auto retObj = callFunc(pWrapFunc->pRealFunc, pWrapFunc->pContext,
		(PyEngObjectPtr)args, (PyEngObjectPtr)kwargs);
	return (PyObject*)retObj;
}

PyObject* CreateFuncWrapper(void* pPyEngHost,
	void* pRealFunc, void* pContext)
{
	PyWrapFunc* pWrapFunc = NewPyWrapFunc(pPyEngHost,pRealFunc, pContext);
	PyObject* retFunc = nullptr;
	static PyMethodDef def{ "XlangPythonWrapper",
		(PyCFunction)XlangPythonWrapper, METH_VARARGS,
		"XlangPythonWrapper" };
	retFunc = PyCFunction_New(&def, (PyObject*)pWrapFunc);
	return retFunc;
}
