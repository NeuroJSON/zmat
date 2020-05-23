/***************************************************************************//**
**  \mainpage ZMat - A portable C-library and MATLAB/Octave toolbox for inline data compression 
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2019-2020
**
**  ZMat provides an easy-to-use interface for stream compression and decompression.
**
**  It can be compiled as a MATLAB/Octave mex function (zipmat.mex/zmat.m) and compresses 
**  arrays and strings in MATLAB/Octave. It can also be compiled as a lightweight
**  C-library (libzmat.a/libzmat.so) that can be called in C/C++/FORTRAN etc to 
**  provide stream-level compression and decompression.
**
**  Currently, zmat/libzmat supports 6 different compression algorthms, including
**     - zlib and gzip : the most widely used algorithm algorithms for .zip and .gz files
**     - lzma and lzip : high compression ratio LZMA based algorithms for .lzma and .lzip files
**     - lz4 and lz4hc : real-time compression based on LZ4 and LZ4HC algorithms
**     - base64        : base64 encoding and decoding
**
**  Depencency: ZLib library: https://www.zlib.net/
**  author: (C) 1995-2017 Jean-loup Gailly and Mark Adler
**
**  Depencency: LZ4 library: https://lz4.github.io/lz4/
**  author: (C) 2011-2019, Yann Collet, 
**
**  Depencency: Original LZMA library
**  author: Igor Pavlov
**
**  Depencency: Eazylzma: https://github.com/lloyd/easylzma
**  author: Lloyd Hilaiel (lloyd)
**
**  Depencency: base64_encode()/base64_decode()
**  \copyright 2005-2011, Jouni Malinen <j@w1.fi>
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/

/***************************************************************************//**
\file    zmatlib.h

@brief   zmat library header file
*******************************************************************************/

#ifndef ZMAT_LIB_H
#define ZMAT_LIB_H

#ifndef NO_LZMA
  #include "easylzma/compress.h"
  #include "easylzma/decompress.h"
#endif

#ifndef NO_LZ4
  #include "lz4/lz4.h"
  #include "lz4/lz4hc.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum TZipMethod {zmZlib, zmGzip, zmBase64, zmLzip, zmLzma, zmLz4, zmLz4hc};

int zmat_run(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret, const int iscompress);
int zmat_encode(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret);
int zmat_decode(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret);

int  zmat_keylookup(char *origkey, const char *table[]);
char *zmat_error(int id);

unsigned char * base64_encode(const unsigned char *src, size_t len,
			      size_t *out_len);
unsigned char * base64_decode(const unsigned char *src, size_t len,
			      size_t *out_len);
#ifndef NO_LZMA
/* compress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int simpleCompress(elzma_file_format format,
                   const unsigned char * inData,
                   size_t inLen,
                   unsigned char ** outData,
                   size_t * outLen,
		   int level);

/* decompress a chunk of memory and return a dynamically allocated buffer
 * if successful.  return value is an easylzma error code */
int simpleDecompress(elzma_file_format format,
                     const unsigned char * inData,
                     size_t inLen,
                     unsigned char ** outData,
                     size_t * outLen);
#endif

#ifdef __cplusplus
}
#endif

#endif
