#include "apr.h"

#ifdef PY_TESTING

static PyObject *pdp6_AprType_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds) {
    PyApr *py_apr = subtype->tp_alloc(suptype, 0);
}

static PyTypeObject pdp6_AprType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_pdp6.Apr",                   /* tp_name */
	sizeof(PyApr),		       /* tp_basicsize */
	0,			       /* tp_itemsize */
	0,			       /* tp_dealloc */
	0,			       /* tp_print */
	0,			       /* tp_getattr */
	0,			       /* tp_setattr */
	0,			       /* tp_reserved */
	0,			       /* tp_repr */
	0,			       /* tp_as_number */
	0,			       /* tp_as_sequence */
	0,			       /* tp_as_mapping */
	0,			       /* tp_hash  */
	0,			       /* tp_call */
	0,			       /* tp_str */
	0,			       /* tp_getattro */
	0,			       /* tp_setattro */
	0,			       /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,            /* tp_flags */
	"PDP-6 simulator state",       /* tp_doc */
    .tp_init = pdp6_AprType_init,
    .tp_new = PyType_GenericNew
};

static PyModuleDef pdp6module = {
	PyModuleDef_HEAD_INIT,
	"_pdp6",
	"PDP-6 simulator backend module",
	-1,
	NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit_pdp6(void)
{
	PyObject* m;

	if (PyType_Ready(&pdp6_AprType) < 0)
		return NULL;

	m = PyModule_Create(&pdp6module);
	if (m == NULL)
		return NULL;

	Py_INCREF(&pdp6_AprType);
	PyModule_AddObject(m, "_pdp6", (PyObject *)&pdp6_AprType);
	return m;
}

PyObject *pdp6_AprType_new(PyTypeObject *subtype, PyObject *args, PyObject *kwargs) {
    PyApr *self = (PyApr *)subtype->tp_alloc(subtype, 0);
    if (!self)
        return NULL;

    if (!(self->apr = apr_init()))
        return Py_DECREF(self), NULL;

    self->begin_cb = self->end_cb = NULL;

    return self;
}

int pdp6_AprType_init(PyObject *self, PyObject *args, PyObject *kwargs) {
    char *kwlist[] = {"begin_cb", "end_cb", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist, &apr->begin_cb, &apr->end_cb))
        return -1;
    PyImport_ImportModule("pdp6")
    return 0;
}

int py_pulse_begin_cb(const char *name, Apr *apr) {
    PyObject *func, *args, *rv, *pyname, *pyapr;

    pyname = PyUnicode_DecodeASCII(name, strlen(name), NULL);

    args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, pyname);
    PyTuple_SetItem(args, 0, apr->py_apr);

    rv = PyObject_CallObject(apr->py_begin_cb, args);
    int ri = PyObject_IsTrue(rv);

    Py_DECREF(args);
    Py_DECREF(func);
    if (rv)
        Py_DECREF(rv);

    return ri;
}

void py_pulse_end_cb(const char *name, Apr *apr) {
    PyObject *func, *args, *rv, *pyname, *pyapr;

    pyname = PyUnicode_DecodeASCII(name, strlen(name), NULL);

    args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, pyname);
    PyTuple_SetItem(args, 0, apr->py_apr);

    rv = PyObject_CallObject(apr->py_end_cb, args);

    Py_DECREF(args);
    Py_DECREF(func);
    if (rv)
        Py_DECREF(rv);
}

#endif /* PY_TESTING */
