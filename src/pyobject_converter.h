
/**
 * PyOtherSide: Asynchronous Python 3 Bindings for Qt 5
 * Copyright (c) 2011, 2013, Thomas Perl <m@thp.io>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 **/

#ifndef PYOTHERSIDE_PYOBJECT_CONVERTER_H
#define PYOTHERSIDE_PYOBJECT_CONVERTER_H

#include "converter.h"

#include "Python.h"

#if PY_MAJOR_VERSION >= 3
#  define PY3K
#endif

class PyObjectListBuilder : public ListBuilder<PyObject *> {
    public:
        PyObjectListBuilder() : list(PyList_New(0)) {}
        virtual ~PyObjectListBuilder() {}

        virtual void append(PyObject *o) {
            PyList_Append(list, o);
        }

        virtual PyObject * value() {
            return list;
        }

    private:
        PyObject *list;
};

class PyObjectDictBuilder : public DictBuilder<PyObject *> {
    public:
        PyObjectDictBuilder() : dict(PyDict_New()) {}
        virtual ~PyObjectDictBuilder() {}

        virtual void set(PyObject *key, PyObject *value) {
            PyDict_SetItem(dict, key, value);
            Py_DECREF(value);
        }

        virtual PyObject * value() {
            return dict;
        }

    private:
        PyObject *dict;
};

class PyObjectListIterator : public ListIterator<PyObject *> {
    public:
        PyObjectListIterator(PyObject *&v) : list(v), pos(0) {}
        virtual ~PyObjectListIterator() {}

        virtual int count() {
            if (PyList_Check(list)) {
                return PyList_Size(list);
            } else {
                return PyTuple_Size(list);
            }
        }

        virtual bool next(PyObject **v) {
            if (pos == count()) {
                return false;
            }

            if (PyList_Check(list)) {
                *v = PyList_GetItem(list, pos);
            } else {
                *v = PyTuple_GetItem(list, pos);
            }

            pos++;
            return true;
        }

    private:
        PyObject *list;
        int pos;
};

class PyObjectDictIterator : public DictIterator<PyObject *> {
    public:
        PyObjectDictIterator(PyObject *&v) : dict(v), pos(0) {}
        virtual ~PyObjectDictIterator() {}

        virtual bool next(PyObject **key, PyObject **value) {
            return PyDict_Next(dict, &pos, key, value);
        }

    private:
        PyObject *dict;
        Py_ssize_t pos;
};



class PyObjectConverter : public Converter<PyObject *> {
    public:
        PyObjectConverter() : stringcontainer(NULL) {}
        virtual ~PyObjectConverter() {
            if (stringcontainer != NULL) {
                Py_DECREF(stringcontainer);
            }
        }

        virtual enum Type type(PyObject *&o) {
            if (PyBool_Check(o)) {
                return BOOLEAN;
#ifdef PY3K
            } else if (PyLong_Check(o)) {
                return INTEGER;
#else
            } else if (PyLong_Check(o) || PyInt_Check(o)) {
                return INTEGER;
#endif
            } else if (PyFloat_Check(o)) {
                return FLOATING;
            } else if (PyUnicode_Check(o) || PyBytes_Check(o)) {
                return STRING;
            } else if (PyList_Check(o) || PyTuple_Check(o)) {
                return LIST;
            } else if (PyDict_Check(o)) {
                return DICT;
            } else if (o == Py_None) {
                return NONE;
            }

            fprintf(stderr, "Warning: Cannot convert:");
            PyObject_Print(o, stderr, 0);
            fprintf(stderr, "\n");

            return NONE;
        }

        virtual long long integer(PyObject *&o) {
#ifdef PY3K
            return PyLong_AsLong(o);
#else
            if (PyInt_Check(o)) {
                return PyInt_AsLong(o);
            } else {
                return PyLong_AsLong(o);
            }
#endif
        }
        virtual double floating(PyObject *&o) { return PyFloat_AsDouble(o); }
        virtual bool boolean(PyObject *&o) { return (o == Py_True); }
        virtual const char *string(PyObject *&o) {
            if (PyBytes_Check(o)) {
                return PyBytes_AsString(o);
            }

            // XXX: In Python 3.3, we can use PyUnicode_UTF8()
            if (stringcontainer != NULL) {
                Py_DECREF(stringcontainer);
            }
            stringcontainer = PyUnicode_AsUTF8String(o);
            return PyBytes_AsString(stringcontainer);
        }
        virtual ListIterator<PyObject *> *list(PyObject *&o) { return new PyObjectListIterator(o); }
        virtual DictIterator<PyObject *> *dict(PyObject *&o) { return new PyObjectDictIterator(o);; }

        virtual PyObject * fromInteger(long long v) { return PyLong_FromLong((long)v); }
        virtual PyObject * fromFloating(double v) { return PyFloat_FromDouble(v); }
        virtual PyObject * fromBoolean(bool v) { return PyBool_FromLong((long)v); }
        virtual PyObject * fromString(const char *v) { return PyUnicode_FromString(v); }
        virtual ListBuilder<PyObject *> *newList() { return new PyObjectListBuilder(); }
        virtual DictBuilder<PyObject *> *newDict() { return new PyObjectDictBuilder(); }
        virtual PyObject * none() { Py_RETURN_NONE; }

    private:
        PyObject *stringcontainer;
};

#endif /* PYOTHERSIDE_PYOBJECT_CONVERTER_H */