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
 * @param nthread: number of threads for blosc2 (default 1)
 * @param shuffle: shuffle flag for blosc2 (default 1)
 * @param typesize: element byte size for blosc2 (default 4)
 * @return bytes object with compressed/decompressed data
 */
static PyObject* pyzmat_zmat(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_buffer input_buf;
    int iscompress = 1;
    const char* method = "zlib";
    int nthread = 1;
    int shuffle = 1;
    int typesize = 4;

    static char* kwlist[] = {"data", "iscompress", "method", "nthread", "shuffle", "typesize", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y*|isi iii", kwlist,
                                     &input_buf, &iscompress, &method,
                                     &nthread, &shuffle, &typesize)) {
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

    int errcode = zmat_run(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        flags.iscompress
    );

    PyBuffer_Release(&input_buf);

    if (errcode < 0) {
        if (outputbuf) {
            free(outputbuf);
        }

        PyErr_Format(PyExc_RuntimeError, "zmat error %d: %s (status=%d)",
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

    int errcode = zmat_run(
        (size_t)input_buf.len,
        (unsigned char*)input_buf.buf,
        &outputsize,
        &outputbuf,
        zipid,
        &ret,
        iscompress
    );

    PyBuffer_Release(&input_buf);

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
     "    method (str): 'zlib','gzip','lzma','lzip','lz4','lz4hc','zstd','base64',\n"
     "                  'blosc2blosclz','blosc2lz4','blosc2lz4hc','blosc2zlib','blosc2zstd'\n"
     "    nthread (int): Thread count for blosc2 (default 1)\n"
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
    "zmat",
    "ZMat - A portable data compression/decompression module\n\n"
    "Supports: zlib, gzip, lzma, lzip, lz4, lz4hc, zstd, blosc2, base64\n\n"
    "Part of the NeuroJSON project (https://neurojson.org)\n"
    "More information: https://github.com/NeuroJSON/zmat\n",
    -1,
    ZmatMethods
};

/* Module initialization */
PyMODINIT_FUNC PyInit_zmat(void) {
    return PyModule_Create(&zmatmodule);
}