/***************************************************************************//**
**  \file    pyzmat.c
**
**  @brief   Python C extension module for ZMat compression/decompression
**
**  \author  Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2019-2025
**
**  Python interface to the zmat compression library.
**
**  Usage:
**      import zmat
**      compressed = zmat.compress(data, method='zlib', level=1)
**      decompressed = zmat.decompress(data, method='zlib')
**      encoded = zmat.encode(data, method='base64')
**      decoded = zmat.decode(data, method='base64')
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "zmatlib.h"

/**
 * Supported method names — must match zipmethods[] in zmatlib
 */
static const char* zipmethods[] = {
    "zlib",
    "gzip",
    "base64",
#if !defined(NO_LZMA)
    "lzip",
    "lzma",
#endif
#if defined(ZMAT_USE_LZMA_SDK)
    "xz",
#endif
#if !defined(NO_LZ4)
    "lz4",
    "lz4hc",
#endif
#if !defined(NO_ZSTD)
    "zstd",
#endif
#if !defined(NO_BLOSC2)
    "blosc2blosclz",
    "blosc2lz4",
    "blosc2lz4hc",
    "blosc2zlib",
    "blosc2zstd",
#endif
    ""
};

static const TZipMethod zipmethodid[] = {
    zmZlib,
    zmGzip,
    zmBase64,
#if !defined(NO_LZMA)
    zmLzip,
    zmLzma,
#endif
#if defined(ZMAT_USE_LZMA_SDK)
    zmXz,
#endif
#if !defined(NO_LZ4)
    zmLz4,
    zmLz4hc,
#endif
#if !defined(NO_ZSTD)
    zmZstd,
#endif
#if !defined(NO_BLOSC2)
    zmBlosc2Blosclz,
    zmBlosc2Lz4,
    zmBlosc2Lz4hc,
    zmBlosc2Zlib,
    zmBlosc2Zstd,
#endif
    zmUnknown
};

/**
 * @brief Look up compression method by name, return TZipMethod enum value
 */
static TZipMethod pyzmat_method_lookup(const char* method) {
    int idx = zmat_keylookup((char*)method, zipmethods);

    if (idx < 0) {
        return zmUnknown;
    }

    return zipmethodid[idx];
}

/**
 * @brief Core function: compress or decompress a buffer
 *
 * zmat.zmat(data, iscompress, method, nthread, shuffle, typesize)
 *
 * @param data: bytes or bytearray input
 * @param iscompress: 1=compress (default), 0=decompress, negative=set level
 * @param method: compression method string (default 'zlib')
 * @param nthread: worker threads. 0, the default, means "unspecified" and lets
 *        the library pick (blosc2 uses 1, zlib/gzip use ZMAT_DEFAULT_NTHREAD);
 *        a negative value forces zlib/gzip's historical single-stream output
 * @param shuffle: shuffle flag for blosc2 (default 1)
 * @param typesize: element byte size for blosc2 (default 4)
 * @param offsets: on decompression, a block index from a previous compression,
 *        which lets zlib/gzip inflate the blocks concurrently. Only a hint: a
 *        mismatched index is rejected in favour of a serial inflate.
 * @param return_offsets: if true, return (bytes, offsets) instead of bytes
 * @return bytes, or (bytes, list-of-int) when return_offsets is set
 */
static PyObject* pyzmat_zmat(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    int iscompress = 1;
    const char* method = "zlib";
    int nthread = 0;
    int shuffle = 1;
    int typesize = 4;
    PyObject* offsets_in = NULL;
    int return_offsets = 0;
    size_t* zoffsets = NULL;
    size_t noffsets = 0;

    static char* kwlist[] = {"data", "iscompress", "method", "nthread", "shuffle",
                             "typesize", "offsets", "return_offsets", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|isiiiOp", kwlist,
                                     &input_buf, &iscompress, &method,
                                     &nthread, &shuffle, &typesize,
                                     &offsets_in, &return_offsets)) {
        return NULL;
    }

    if (input_buf.len == 0) {
        PyBuffer_Release(&input_buf);
        return PyBytes_FromStringAndSize("", 0);
    }

    TZipMethod zipid = pyzmat_method_lookup(method);

    if (zipid == zmUnknown) {
        PyBuffer_Release(&input_buf);
        PyErr_Format(PyExc_ValueError, "unsupported compression method '%s'", method);
        return NULL;
    }

    /* pack flags the same way as zmat.cpp / zmatlib.c */
    union TZMatFlags flags = {0};
    flags.param.clevel = (char)iscompress;
    flags.param.nthread = (char)nthread;
    flags.param.shuffle = (char)shuffle;
    flags.param.typesize = (char)typesize;

    unsigned char* outputbuf = NULL;
    size_t outputsize = 0;
    int ret = 0;

    /* an index supplied by the caller is only useful when decompressing */
    if (offsets_in && offsets_in != Py_None && !flags.param.clevel) {
        PyObject* seq = PySequence_Fast(offsets_in, "offsets must be a sequence of integers");

        if (!seq) {
            PyBuffer_Release(&input_buf);
            return NULL;
        }

        Py_ssize_t cnt = PySequence_Fast_GET_SIZE(seq);

        if (cnt >= 4 && !(cnt & 1)) {
            zoffsets = (size_t*)malloc((size_t)cnt * sizeof(size_t));

            if (zoffsets) {
                Py_ssize_t k;

                for (k = 0; k < cnt; k++) {
                    zoffsets[k] = (size_t)PyLong_AsUnsignedLongLong(PySequence_Fast_GET_ITEM(seq, k));
                }

                if (PyErr_Occurred()) {
                    free(zoffsets);
                    Py_DECREF(seq);
                    PyBuffer_Release(&input_buf);
                    return NULL;
                }

                noffsets = (size_t)cnt;
            }
        }

        Py_DECREF(seq);
    }

    int errcode = zmat_run_indexed(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        flags.iscompress,
        &zoffsets,
        &noffsets
    );

    PyBuffer_Release(&input_buf);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        free(zoffsets);
        PyErr_Format(PyExc_RuntimeError, "zmat error %d: %s (status=%d)",
                     errcode, zmat_error(-errcode), ret);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize((const char*)outputbuf, outputsize);

    if (outputbuf) {
        free(outputbuf);
    }

    if (return_offsets) {
        PyObject* olist;

        if (result && zoffsets && noffsets >= 4) {
            size_t k;
            olist = PyList_New((Py_ssize_t)noffsets);

            if (olist) {
                for (k = 0; k < noffsets; k++) {
                    PyList_SET_ITEM(olist, (Py_ssize_t)k,
                                    PyLong_FromUnsignedLongLong((unsigned long long)zoffsets[k]));
                }
            }
        } else {
            olist = PyList_New(0);
        }

        free(zoffsets);

        if (!result || !olist) {
            Py_XDECREF(result);
            Py_XDECREF(olist);
            return NULL;
        }

        return Py_BuildValue("(NN)", result, olist);
    }

    free(zoffsets);
    return result;
}

/**
 * @brief Convenience function: compress data
 *
 * zmat.compress(data, method='zlib', level=1)
 */
static PyObject* pyzmat_compress(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    const char* method = "zlib";
    int level = 1;

    static char* kwlist[] = {"data", "method", "level", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|si", kwlist,
                                     &input_buf, &method, &level)) {
        return NULL;
    }

    if (input_buf.len == 0) {
        PyBuffer_Release(&input_buf);
        return PyBytes_FromStringAndSize("", 0);
    }

    TZipMethod zipid = pyzmat_method_lookup(method);

    if (zipid == zmUnknown) {
        PyBuffer_Release(&input_buf);
        PyErr_Format(PyExc_ValueError, "unsupported compression method '%s'", method);
        return NULL;
    }

    int iscompress = (level >= 1) ? 1 : -level;

    unsigned char* outputbuf = NULL;
    size_t outputsize = 0;
    int ret = 0;

    /* NULL index: no block index is produced or consumed here, but routing
     * through the indexed entry point is what lets zlib and gzip use the
     * build-time default thread count */
    size_t* zoffsets = NULL;
    size_t noffsets = 0;

    int errcode = zmat_run_indexed(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        iscompress,
        &zoffsets,
        &noffsets
    );

    PyBuffer_Release(&input_buf);

    free(zoffsets);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        PyErr_Format(PyExc_RuntimeError, "zmat compression error %d: %s (status=%d)",
                     errcode, zmat_error(-errcode), ret);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize((const char*)outputbuf, outputsize);

    if (outputbuf) {
        free(outputbuf);
    }

    return result;
}

/**
 * @brief Convenience function: decompress data
 *
 * zmat.decompress(data, method='zlib')
 */
static PyObject* pyzmat_decompress(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    const char* method = "zlib";

    static char* kwlist[] = {"data", "method", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|s", kwlist,
                                     &input_buf, &method)) {
        return NULL;
    }

    if (input_buf.len == 0) {
        PyBuffer_Release(&input_buf);
        return PyBytes_FromStringAndSize("", 0);
    }

    TZipMethod zipid = pyzmat_method_lookup(method);

    if (zipid == zmUnknown) {
        PyBuffer_Release(&input_buf);
        PyErr_Format(PyExc_ValueError, "unsupported compression method '%s'", method);
        return NULL;
    }

    unsigned char* outputbuf = NULL;
    size_t outputsize = 0;
    int ret = 0;

    /* NULL index: no block index is produced or consumed here, but routing
     * through the indexed entry point is what lets zlib and gzip use the
     * build-time default thread count */
    size_t* zoffsets = NULL;
    size_t noffsets = 0;

    int errcode = zmat_run_indexed(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        0,
        &zoffsets,
        &noffsets
    );

    PyBuffer_Release(&input_buf);

    free(zoffsets);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        PyErr_Format(PyExc_RuntimeError, "zmat decompression error %d: %s (status=%d)",
                     errcode, zmat_error(-errcode), ret);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize((const char*)outputbuf, outputsize);

    if (outputbuf) {
        free(outputbuf);
    }

    return result;
}

/**
 * @brief Convenience function: base64 encode
 *
 * zmat.encode(data, method='base64')
 */
static PyObject* pyzmat_encode(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    const char* method = "base64";

    static char* kwlist[] = {"data", "method", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|s", kwlist,
                                     &input_buf, &method)) {
        return NULL;
    }

    if (input_buf.len == 0) {
        PyBuffer_Release(&input_buf);
        return PyBytes_FromStringAndSize("", 0);
    }

    TZipMethod zipid = pyzmat_method_lookup(method);

    if (zipid == zmUnknown) {
        PyBuffer_Release(&input_buf);
        PyErr_Format(PyExc_ValueError, "unsupported method '%s'", method);
        return NULL;
    }

    unsigned char* outputbuf = NULL;
    size_t outputsize = 0;
    int ret = 0;

    int errcode = zmat_run(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        1
    );

    PyBuffer_Release(&input_buf);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        PyErr_Format(PyExc_RuntimeError, "zmat encode error %d: %s (status=%d)",
                     errcode, zmat_error(-errcode), ret);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize((const char*)outputbuf, outputsize);

    if (outputbuf) {
        free(outputbuf);
    }

    return result;
}

/**
 * @brief Convenience function: base64 decode
 *
 * zmat.decode(data, method='base64')
 */
static PyObject* pyzmat_decode(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    const char* method = "base64";

    static char* kwlist[] = {"data", "method", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|s", kwlist,
                                     &input_buf, &method)) {
        return NULL;
    }

    if (input_buf.len == 0) {
        PyBuffer_Release(&input_buf);
        return PyBytes_FromStringAndSize("", 0);
    }

    TZipMethod zipid = pyzmat_method_lookup(method);

    if (zipid == zmUnknown) {
        PyBuffer_Release(&input_buf);
        PyErr_Format(PyExc_ValueError, "unsupported method '%s'", method);
        return NULL;
    }

    unsigned char* outputbuf = NULL;
    size_t outputsize = 0;
    int ret = 0;

    int errcode = zmat_run(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        0
    );

    PyBuffer_Release(&input_buf);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        PyErr_Format(PyExc_RuntimeError, "zmat decode error %d: %s (status=%d)",
                     errcode, zmat_error(-errcode), ret);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize((const char*)outputbuf, outputsize);

    if (outputbuf) {
        free(outputbuf);
    }

    return result;
}

/* Module method table */
static PyMethodDef ZmatMethods[] = {
    {"zmat",       (PyCFunction)pyzmat_zmat,       METH_VARARGS | METH_KEYWORDS,
     "zmat(data, iscompress=1, method='zlib', nthread=1, shuffle=1, typesize=4)\n\n"
     "Low-level compression/decompression interface.\n\n"
     "Args:\n"
     "    data (bytes): Input data buffer\n"
     "    iscompress (int): 1=compress, 0=decompress, negative=set compression level\n"
     "    method (str): 'zlib','gzip','lzma','lzip','xz','lz4','lz4hc','zstd','base64',\n"
     "                  'blosc2blosclz','blosc2lz4','blosc2lz4hc','blosc2zlib','blosc2zstd'\n"
     "    nthread (int): Thread count for lzip, xz, zstd, and blosc2 (default 1)\n"
     "    shuffle (int): Shuffle flag for blosc2 (default 1)\n"
     "    typesize (int): Element byte size for blosc2 shuffle (default 4)\n\n"
     "Returns:\n"
     "    bytes: Compressed or decompressed data"},

    {"compress",   (PyCFunction)pyzmat_compress,   METH_VARARGS | METH_KEYWORDS,
     "compress(data, method='zlib', level=1)\n\n"
     "Compress data using the specified method.\n\n"
     "Args:\n"
     "    data (bytes): Input data to compress\n"
     "    method (str): Compression method (default 'zlib')\n"
     "    level (int): Compression level, 1=default, higher=more compression\n\n"
     "Returns:\n"
     "    bytes: Compressed data"},

    {"decompress", (PyCFunction)pyzmat_decompress, METH_VARARGS | METH_KEYWORDS,
     "decompress(data, method='zlib')\n\n"
     "Decompress data using the specified method.\n\n"
     "Args:\n"
     "    data (bytes): Compressed input data\n"
     "    method (str): Compression method used (default 'zlib')\n\n"
     "Returns:\n"
     "    bytes: Decompressed data"},

    {"encode",     (PyCFunction)pyzmat_encode,     METH_VARARGS | METH_KEYWORDS,
     "encode(data, method='base64')\n\n"
     "Encode data (e.g. base64 encoding).\n\n"
     "Args:\n"
     "    data (bytes): Input data to encode\n"
     "    method (str): Encoding method (default 'base64')\n\n"
     "Returns:\n"
     "    bytes: Encoded data"},

    {"decode",     (PyCFunction)pyzmat_decode,     METH_VARARGS | METH_KEYWORDS,
     "decode(data, method='base64')\n\n"
     "Decode data (e.g. base64 decoding).\n\n"
     "Args:\n"
     "    data (bytes): Encoded input data\n"
     "    method (str): Encoding method used (default 'base64')\n\n"
     "Returns:\n"
     "    bytes: Decoded data"},

    {NULL, NULL, 0, NULL}
};

/* Module definition */
static struct PyModuleDef zmatmodule = {
    PyModuleDef_HEAD_INIT,
    "_zmat",
    "ZMat (1.2.preview) — use the 'zmat' package, not this module directly.\n\n"
    "Supports: zlib, gzip, lzma, lzip, xz, lz4, lz4hc, zstd, blosc2, base64\n\n"
    "Part of the NeuroJSON project (https://neurojson.org)\n"
    "More information: https://neurojson.org/zmat\n",
    -1,
    ZmatMethods
};

/* Module initialization */
PyMODINIT_FUNC PyInit__zmat(void) {
    return PyModule_Create(&zmatmodule);
}