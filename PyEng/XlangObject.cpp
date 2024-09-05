#include "PyFunc.h"
#include "PyEngHostImpl.h"
#include <unordered_map>
#include <string>
#include "xlang.h"
#include "xhost.h"
#include "PyObjectXLangConverter.h"

typedef struct {
    PyObject_HEAD
    X::Value realObj;
    //X::Runtime rt;
} PyXlangObject;

PyXlangObject* NewXlangObject(X::Value& realObj);


static PyObject* XlangObject_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    PyXlangObject* self = (PyXlangObject*)type->tp_alloc(type, 0);
    if (self != nullptr) {
        //add init here
    }
    return (PyObject*)self;
}

static int XlangObject_init(PyXlangObject* self, PyObject* args, PyObject* kwds) {
    return 0;
}

static void XlangObject_dealloc(PyXlangObject* self) {

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject* XlangObject_getattr(PyXlangObject* self, PyObject* name) {
    const char* attr_name = PyUnicode_AsUTF8(name);
    X::Value newObj = self->realObj[attr_name];
    if (newObj.IsInvalid())
    {
        Py_IncRef(Py_None);      
        return Py_None;
    }
    PyXlangObject* new_obj = NewXlangObject(newObj);
    return (PyObject*)new_obj;
}

static int XlangObject_setattr(PyXlangObject* self, PyObject* name, PyObject* value) {
    const char* attr_name = PyUnicode_AsUTF8(name);
    return 0;
}

static PyObject* XlangObject_getitem(PyXlangObject* self, PyObject* key) {
    const char* key_str = PyUnicode_AsUTF8(key);
    PyErr_SetString(PyExc_KeyError, "Key not found");
    return nullptr;
}

static int XlangObject_setitem(PyXlangObject* self, PyObject* key, PyObject* value) {
    const char* key_str = PyUnicode_AsUTF8(key);
    return 0;
}

static PyObject* XlangObject_call(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyXlangObject* pXlangObject = (PyXlangObject*)self;
    X::Value realObj = pXlangObject->realObj;
    if(realObj.IsObject())
	{
        X::ARGS x_args(0);
        if(args != nullptr)
		{
			PyObjectXLangConverter::ConvertArgs(args, x_args);
		}
        X::KWARGS x_kwargs;
        if (kwargs != nullptr)
        {
            PyObjectXLangConverter::ConvertKwargs(kwargs, x_kwargs);
        }
        X::Value retVal = realObj.ObjCall(x_args, x_kwargs);
        PyObject* retObj = PyObjectXLangConverter::ConvertToPyObject(retVal);
        return retObj;
	}
	return nullptr;
}

static PyMappingMethods XlangObject_as_mapping = {
    0,                                  /* mp_length */
    (binaryfunc)XlangObject_getitem,    /* mp_subscript */
    (objobjargproc)XlangObject_setitem, /* mp_ass_subscript */
};

static PyTypeObject XlangObjectType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "XlangObject",                      /* tp_name */
    sizeof(PyXlangObject),              /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)XlangObject_dealloc,    /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    &XlangObject_as_mapping,            /* tp_as_mapping */
    0,                                  /* tp_hash  */
    XlangObject_call,                   /* tp_call */
    0,                                  /* tp_str */
    (getattrofunc)XlangObject_getattr,  /* tp_getattro */
    (setattrofunc)XlangObject_setattr,  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    "Xlang Python Object",              /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)XlangObject_init,         /* tp_init */
    0,                                  /* tp_alloc */
    XlangObject_new,                    /* tp_new */
    0,                                  /* tp_free */
    0,                                  /* tp_is_gc */
    0,                                  /* tp_bases */
    0,                                  /* tp_mro */
    0,                                  /* tp_cache */
    0,                                  /* tp_subclasses */
    0,                                  /* tp_weaklist */
    0,                                  /* tp_del */
    0,                                  /* tp_version_tag */
    0,                                  /* tp_finalize */
    0,                                  /* tp_vectorcall */
};

static bool __XlangObjectType_Prepared = false;
void PrepareXlangObjectType() {
    PyType_Ready(&XlangObjectType);
    __XlangObjectType_Prepared = true;
}

PyXlangObject* NewXlangObject(X::Value& realObj) {
    if (!__XlangObjectType_Prepared) {
        PrepareXlangObjectType();
    }
    auto self = (PyXlangObject*)XlangObjectType.tp_alloc(&XlangObjectType, 0);
    self->realObj = realObj;
    return self;
}

void DestroyXlangObject(PyXlangObject* self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* CreateXlangObjectWrapper(X::Value& realObj) {
    PyXlangObject* pXlangObject = NewXlangObject(realObj);
    return (PyObject*)pXlangObject;
}
