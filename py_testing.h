#ifndef __PY_TESTING_H__
#define __PY_TESTING_H__

#include <Python.h>
#include "pdp6.h"

#ifdef PY_TESTING
struct Apr;

typedef struct {
    PyObject_HEAD
    PyObject *py_begin_cb, *py_end_cb;
    struct Apr *apr;
} PyApr;

#endif /* PY_TESTING */
#endif /* __PY_TESTING_H__ */
