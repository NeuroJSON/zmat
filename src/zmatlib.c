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
\file    zmatlib.c

@brief   Compression and decompression interfaces: zmat_run, zmat_encode, zmat_decode
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "zmatlib.h"

#ifndef NO_ZLIB
    #include "zlib.h"
#else
    #include "miniz.h"
#endif

#ifndef NO_LZMA
    #include "easylzma/compress.h"
    #include "easylzma/decompress.h"
#endif

#ifndef NO_LZ4
    #include "lz4/lz4.h"
    #include "lz4/lz4hc.h"
#endif

#ifndef NO_BLOSC2
    #include "blosc2.h"
#endif

#ifndef NO_ZSTD
    #include "zstd.h"
    unsigned long long ZSTD_decompressBound(const void* src, size_t srcSize);
#endif


#ifndef NO_LZMA
/**
 * @brief Easylzma interface to perform compression
 *
 * @param[in] format: output format (0 for lzip format, 1 for lzma-alone format)
 * @param[in] inData: input stream buffer pointer
 * @param[in] inLen: input stream buffer length
 * @param[in] outData: output stream buffer pointer
 * @param[in] outLen: output stream buffer length
 * @param[in] level: positive number: use default compression level (5);
 *             negative interger: set compression level (-1, less, to -9, more compression)
 * @return return the fine grained lzma error code.
 */

int simpleCompress(elzma_file_format format,
                   const unsigned char* inData,
                   size_t inLen,
                   unsigned char** outData,
                   size_t* outLen,
                   int level);

/**
 * @brief Easylzma interface to perform decompression
 *
 * @param[in] format: output format (0 for lzip format, 1 for lzma-alone format)
 * @param[in] inData: input stream buffer pointer
 * @param[in] inLen: input stream buffer length
 * @param[in] outData: output stream buffer pointer
 * @param[in] outLen: output stream buffer length
 * @return return the fine grained lzma error code.
 */

int simpleDecompress(elzma_file_format format,
                     const unsigned char* inData,
                     size_t inLen,
                     unsigned char** outData,
                     size_t* outLen);
#endif

/**
 * @brief Coarse grained error messages (encoder-specific detailed error codes are in the status parameter)
 *
 */

const char* zmat_errcode[] = {
    "No error", /*0*/
    "input can not be empty", /*-1*/
    "failed to initialize zlib", /*-2*/
    "zlib error, see info.status for error flag, often a result of mismatch in compression method", /*-3*/
    "easylzma error, see info.status for error flag, often a result of mismatch in compression method",/*-4*/
    "can not allocate output buffer",/*-5*/
    "lz4 error, see info.status for error flag, often a result of mismatch in compression method",/*-6*/
    "unsupported blosc2 codec",/*-7*/
    "blosc2 error, see info.status for error flag, often a result of mismatch in compression method",/*-8*/
    "zstd error, see info.status for error flag, often a result of mismatch in compression method",/*-9*/
    "unsupported method" /*-999*/
};

/**
 * @brief Convert error code to a string error message
 *
 * @param[in] id: zmat error code
 */

char* zmat_error(int id) {
    if (id >= 0 && id < (sizeof(zmat_errcode) / sizeof(zmat_errcode[0]))) {
        return (char*)(zmat_errcode[id]);
    } else {
        return "zmatlib: unknown error";
    }
}

/**
 * @brief Main interface to perform compression/decompression
 *
 * @param[in] inputsize: input stream buffer length
 * @param[in] inputstr: input stream buffer pointer
 * @param[in, out] outputsize: output stream buffer length
 * @param[in, out] outputbuf: output stream buffer pointer
 * @param[out] ret: encoder/decoder specific detailed error code (if error occurs)
 * @param[in] iscompress: 0: decompression, 1: use default compression level;
 *             negative interger: set compression level (-1, less, to -9, more compression)
 * @return return the coarse grained zmat error code; detailed error code is in ret.
 */

int zmat_run(const size_t inputsize, unsigned char* inputstr, size_t* outputsize, unsigned char** outputbuf, const int zipid, int* ret, const int iscompress) {
    z_stream zs;
    size_t buflen[2] = {0};
    unsigned int nthread = 1, shuffle = 1, typesize = 4;
    int clevel;
    union cflag {
        int iscompress;
        struct settings {
            char clevel;
            char nthread;
            char shuffle;
            char typesize;
        } param;
    } flags;
    *outputbuf = NULL;

    flags.iscompress = iscompress;

    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (inputsize == 0) {
        return -1;
    }

    nthread = (flags.param.nthread == 0 || flags.param.nthread == -1) ? 1 : flags.param.nthread;
    shuffle = (flags.param.shuffle == 0 || flags.param.shuffle == -1) ? 1 : flags.param.shuffle;
    typesize = (flags.param.typesize == 0 || flags.param.typesize == -1) ? 4 : flags.param.typesize;
    clevel = (flags.param.clevel == 0) ? 0 : flags.param.clevel;

    if (clevel) {
        /**
          * perform compression or encoding
          */
        if (zipid == zmBase64) {
            /**
              * base64 encoding
              */
            *outputbuf = base64_encode((const unsigned char*)inputstr, inputsize, outputsize, clevel);
        } else if (zipid == zmZlib || zipid == zmGzip) {
            /**
              * zlib (.zip) or gzip (.gz) compression
              */
            if (zipid == zmZlib) {
                if (deflateInit(&zs,  (clevel > 0) ? Z_DEFAULT_COMPRESSION : (-clevel)) != Z_OK) {
                    return -2;
                }
            } else {
                if (deflateInit2(&zs, (clevel > 0) ? Z_DEFAULT_COMPRESSION : (-clevel), Z_DEFLATED, 15 | 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) {
                    return -2;
                }
            }

            buflen[0] = deflateBound(&zs, inputsize);
            *outputbuf = (unsigned char*)malloc(buflen[0]);
            zs.avail_in = inputsize; /* size of input, string + terminator*/
            zs.next_in = (Bytef*)inputstr;  /* input char array*/
            zs.avail_out = buflen[0]; /* size of output*/

            zs.next_out =  (Bytef*)(*outputbuf);  /*(Bytef *)(); // output char array*/

            *ret = deflate(&zs, Z_FINISH);
            *outputsize = zs.total_out;

            if (*ret != Z_STREAM_END && *ret != Z_OK) {
                deflateEnd(&zs);
                return -3;
            }

            deflateEnd(&zs);
#ifndef NO_LZMA
        } else if (zipid == zmLzma || zipid == zmLzip) {
            /**
              * lzma (.lzma) or lzip (.lzip) compression
              */
            *ret = simpleCompress((elzma_file_format)(zipid - 3), (unsigned char*)inputstr,
                                  inputsize, outputbuf, outputsize, clevel);

            if (*ret != ELZMA_E_OK) {
                return -4;
            }

#endif
#ifndef NO_LZ4
        } else if (zipid == zmLz4 || zipid == zmLz4hc) {
            /**
              * lz4 or lz4hc compression
              */
            *outputsize = LZ4_compressBound(inputsize);

            if (!(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                return -5;
            }

            if (zipid == zmLz4) {
                *outputsize = LZ4_compress_default((const char*)inputstr, (char*)(*outputbuf), inputsize, *outputsize);
            } else {
                *outputsize = LZ4_compress_HC((const char*)inputstr, (char*)(*outputbuf), inputsize, *outputsize, (clevel > 0) ? 8 : (-clevel));
            }

            *ret = *outputsize;

            if (*outputsize == 0) {
                return -6;
            }

#endif
#ifndef NO_ZSTD
        } else if (zipid == zmZstd) {
            /**
              * zstd compression
              */
            *outputsize = ZSTD_compressBound(inputsize);

            if (!(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                return -5;
            }

            *ret = ZSTD_compress((char*)(*outputbuf), *outputsize, (const char*)inputstr, inputsize, (clevel > 0) ? ZSTD_CLEVEL_DEFAULT : (-clevel));

            if (ZSTD_isError(*ret)) {
                return -9;
            }

            *outputsize = *ret;

            if (!(*outputbuf = (unsigned char*)realloc(*outputbuf, *outputsize))) {
                return -5;
            }

#endif
#ifndef NO_BLOSC2
        } else if (zipid >= zmBlosc2Blosclz && zipid <= zmBlosc2Zstd) {
            /**
              * blosc2 meta-compressor (support various filters and compression codecs)
              */
            const char* codecs[] = {"blosclz", "lz4", "lz4hc", "zlib", "zstd"};

            if (blosc1_set_compressor(codecs[zipid - zmBlosc2Blosclz]) == -1) {
                return -7;
            }

            blosc2_set_nthreads(nthread);

            *outputsize = inputsize + BLOSC2_MAX_OVERHEAD;  /* blosc2 guarantees the compression will always succeed at this size */

            if (!(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                return -5;
            }

            *ret = blosc1_compress((clevel > 0) ? 5 : (-clevel), shuffle, typesize, inputsize, (const void*)inputstr, (void*)(*outputbuf), *outputsize);

            if (*ret < 0) {
                return -8;
            }

            *outputsize = *ret;

            if (!(*outputbuf = (unsigned char*)realloc(*outputbuf, *outputsize))) {
                return -5;
            }

#endif
        } else {
            return -999;
        }
    } else {
        /**
          * perform decompression or decoding
          */
        if (zipid == zmBase64) {
            /**
              * base64 decoding
              */
            *outputbuf = base64_decode((const unsigned char*)inputstr, inputsize, outputsize);
        } else if (zipid == zmZlib || zipid == zmGzip) {
            /**
              * zlib (.zip) or gzip (.gz) decompression
              */
            int count = 1;
#ifdef NO_ZLIB
            tinfl_decompressor inflator;
#endif

            if (zipid == zmZlib) {
                if (inflateInit(&zs) != Z_OK) {
                    return -2;
                }
            } else {
#ifdef NO_ZLIB
                tinfl_init(&inflator);
#else

                if (inflateInit2(&zs, 15 | 32) != Z_OK) {
                    return -2;
                }

#endif
            }

            buflen[0] = inputsize * 20;
            *outputbuf = (unsigned char*)malloc(buflen[0]);

            zs.avail_in = inputsize; /* size of input, string + terminator*/
            zs.next_in = inputstr; /* input char array*/
            zs.avail_out = buflen[0]; /* size of output*/

            zs.next_out =  (Bytef*)(*outputbuf);  /* output char array*/

#ifdef NO_ZLIB

            if (zipid == zmZlib) {
#endif

                while ((*ret = inflate(&zs, Z_SYNC_FLUSH)) != Z_STREAM_END && *ret != Z_DATA_ERROR && count <= 10) {
                    *outputbuf = (unsigned char*)realloc(*outputbuf, (buflen[0] << count));
                    zs.next_out =  (Bytef*)(*outputbuf + (buflen[0] << (count - 1)));
                    zs.avail_out = (buflen[0] << (count - 1)); /* size of output*/
                    count++;
                }

                *outputsize = zs.total_out;
#ifdef NO_ZLIB
            } else {

                size_t insize = inputsize;

                while ((*ret = tinfl_decompress(&inflator, inputstr + 10, &insize, *outputbuf, *outputbuf, outputsize, 0)) != TINFL_STATUS_DONE && *ret != Z_DATA_ERROR && count <= 10) {
                    *outputbuf = (unsigned char*)realloc(*outputbuf, (buflen[0] << count));
                    zs.next_out =  (Bytef*)(*outputbuf + (buflen[0] << (count - 1)));
                    zs.avail_out = (buflen[0] << (count - 1)); /* size of output*/
                    count++;
                }
            }

#endif

            if (*ret != Z_STREAM_END && *ret != Z_OK) {
                inflateEnd(&zs);
                return -3;
            }

            inflateEnd(&zs);
#ifndef NO_LZMA
        } else if (zipid == zmLzma || zipid == zmLzip) {
            /**
              * lzma (.lzma) or lzip (.lzip) decompression
              */
            *ret = simpleDecompress((elzma_file_format)(zipid - 3), (unsigned char*)inputstr,
                                    inputsize, outputbuf, outputsize);

            if (*ret != ELZMA_E_OK) {
                return -4;
            }

#endif
#ifndef NO_LZ4
        } else if (zipid == zmLz4 || zipid == zmLz4hc) {
            /**
              * lz4 or lz4hc decompression
              */
            int count = 2;
            *outputsize = (inputsize << count);

            if (!(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                *ret = -5;
                return *ret;
            }

            while ((*ret = LZ4_decompress_safe((const char*)inputstr, (char*)(*outputbuf), inputsize, *outputsize)) <= 0 && count <= 10) {
                *outputsize = (inputsize << count);

                if (!(*outputbuf = (unsigned char*)realloc(*outputbuf, *outputsize))) {
                    *ret = -5;
                    return *ret;
                }

                count++;
            }

            *outputsize = *ret;

            if (*ret < 0) {
                return -6;
            }

#endif
#ifndef NO_ZSTD
        } else if (zipid == zmZstd) {
            /**
              * zstd decompression
              */
            *outputsize = ZSTD_decompressBound(inputstr, inputsize);

            if (*outputsize == ZSTD_CONTENTSIZE_ERROR || !(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                *ret = (*outputsize == ZSTD_CONTENTSIZE_ERROR) ? -9 : -5;
                return *ret;
            }

            *ret = ZSTD_decompress((void*)(*outputbuf), *outputsize, (const void*)inputstr, inputsize);

            *outputsize = *ret;

            if (ZSTD_isError(*ret)) {
                return -9;
            }

#endif
#ifndef NO_BLOSC2
        } else if (zipid >= zmBlosc2Blosclz && zipid <= zmBlosc2Zstd) {
            /**
              * blosc2 meta-compressor (support various filters and compression codecs)
              */
            int count = 2;
            *outputsize = (inputsize << count);

            if (!(*outputbuf = (unsigned char*)malloc(*outputsize))) {
                *ret = -5;
                return *ret;
            }

            while ((*ret = blosc1_decompress((const char*)inputstr, (char*)(*outputbuf), *outputsize)) <= 0 && count <= 16) {
                *outputsize = (inputsize << count);

                if (!(*outputbuf = (unsigned char*)realloc(*outputbuf, *outputsize))) {
                    *ret = -5;
                    return *ret;
                }

                count++;
            }

            *outputsize = *ret;

            if (*ret < 0) {
                return -8;
            }

#endif
        } else {
            return -999;
        }
    }

    return 0;
}

/**
 * @brief Simplified interface to perform compression (use default compression level)
 *
 * @param[in] inputsize: input stream buffer length
 * @param[in] inputstr: input stream buffer pointer
 * @param[in] outputsize: output stream buffer length
 * @param[in] outputbuf: output stream buffer pointer
 * @param[in] ret: encoder/decoder specific detailed error code (if error occurs)
 * @return return the coarse grained zmat error code; detailed error code is in ret.
 */

int zmat_encode(const size_t inputsize, unsigned char* inputstr, size_t* outputsize, unsigned char** outputbuf, const int zipid, int* ret) {
    return zmat_run(inputsize, inputstr, outputsize, outputbuf, zipid, ret, 1);
}

/**
 * @brief Simplified interface to perform decompression
 *
 * @param[in] inputsize: input stream buffer length
 * @param[in] inputstr: input stream buffer pointer
 * @param[in] outputsize: output stream buffer length
 * @param[in] outputbuf: output stream buffer pointer
 * @param[in] ret: encoder/decoder specific detailed error code (if error occurs)
 * @return return the coarse grained zmat error code; detailed error code is in ret.
 */

int zmat_decode(const size_t inputsize, unsigned char* inputstr, size_t* outputsize, unsigned char** outputbuf, const int zipid, int* ret) {
    return zmat_run(inputsize, inputstr, outputsize, outputbuf, zipid, ret, 0);
}

/**
 * @brief Look up a string in a string list and return the index
 *
 * @param[in] origkey: string to be looked up
 * @param[out] table: the dictionary where the string is searched
 * @return if found, return the index of the string in the dictionary, otherwise -1.
 */

int zmat_keylookup(char* origkey, const char* table[]) {
    int i = 0;
    char* key = (char*)malloc(strlen(origkey) + 1);
    memcpy(key, origkey, strlen(origkey) + 1);

    while (key[i]) {
        key[i] = tolower(key[i]);
        i++;
    }

    i = 0;

    while (table[i] && table[i][0] != '\0') {
        if (strcmp(key, table[i]) == 0) {
            free(key);
            return i;
        }

        i++;
    }

    free(key);
    return -1;
}

/**
 * @brief Free the output buffer to facilitate use in fortran
 *
 * @param[in,out] outputbuf: the outputbuf buffer's initial address to be freed
 */

void zmat_free(unsigned char** outputbuf) {
    if (*outputbuf) {
        free(*outputbuf);
    }

    *outputbuf = NULL;
}

/*
 * @brief Base64 encoding/decoding (RFC1341)
 * @author Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

static const unsigned char base64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */

unsigned char* base64_encode(const unsigned char* src, size_t len,
                             size_t* out_len, int mode) {
    unsigned char* out, *pos;
    const unsigned char* end, *in;
    size_t olen;
    size_t line_len;

    olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
    olen += olen / 72; /* line feeds */
    olen++; /* nul termination */

    if (olen < len) {
        return NULL;    /* integer overflow */
    }

    out = (unsigned char*)malloc(olen);

    if (out == NULL) {
        return NULL;
    }

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;

    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;

        if (mode < 3 && line_len >= 72) {
            *pos++ = '\n';
            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];

        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) |
                                                  (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }

        *pos++ = '=';
        line_len += 4;
    }

    if (mode < 2 && line_len) {
        *pos++ = '\n';
    }

    *pos = '\0';

    if (out_len) {
        *out_len = pos - out;
    }

    return out;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */

unsigned char* base64_decode(const unsigned char* src, size_t len,
                             size_t* out_len) {
    unsigned char dtable[256], *out, *pos, block[4], tmp;
    size_t i, count, olen;
    int pad = 0;

    memset(dtable, 0x80, 256);

    for (i = 0; i < sizeof(base64_table) - 1; i++) {
        dtable[base64_table[i]] = (unsigned char) i;
    }

    dtable['='] = 0;

    count = 0;

    for (i = 0; i < len; i++) {
        if (dtable[src[i]] != 0x80) {
            count++;
        }
    }

    if (count == 0 || count % 4) {
        return NULL;
    }

    olen = count / 4 * 3;
    pos = out = (unsigned char*)malloc(olen);

    if (out == NULL) {
        return NULL;
    }

    count = 0;

    for (i = 0; i < len; i++) {
        tmp = dtable[src[i]];

        if (tmp == 0x80) {
            continue;
        }

        if (src[i] == '=') {
            pad++;
        }

        block[count] = tmp;
        count++;

        if (count == 4) {
            *pos++ = (block[0] << 2) | (block[1] >> 4);
            *pos++ = (block[1] << 4) | (block[2] >> 2);
            *pos++ = (block[2] << 6) | block[3];
            count = 0;

            if (pad) {
                if (pad == 1) {
                    pos--;
                } else if (pad == 2) {
                    pos -= 2;
                } else {
                    /* Invalid padding */
                    free(out);
                    return NULL;
                }

                break;
            }
        }
    }

    *out_len = pos - out;
    return out;
}

#ifndef NO_LZMA

/**
 * @brief Easylzma compression interface
 */

struct dataStream {
    const unsigned char* inData;
    size_t inLen;

    unsigned char* outData;
    size_t outLen;
};

/**
 * @brief Easylzma input callback function
 */

static int
inputCallback(void* ctx, void* buf, size_t* size) {
    size_t rd = 0;
    struct dataStream* ds = (struct dataStream*) ctx;
    assert(ds != NULL);

    rd = (ds->inLen < *size) ? ds->inLen : *size;

    if (rd > 0) {
        memcpy(buf, (void*) ds->inData, rd);
        ds->inData += rd;
        ds->inLen -= rd;
    }

    *size = rd;

    return 0;
}

/**
 * @brief Easylzma output callback function
 */

static size_t
outputCallback(void* ctx, const void* buf, size_t size) {
    struct dataStream* ds = (struct dataStream*) ctx;
    assert(ds != NULL);

    if (size > 0) {
        ds->outData = (unsigned char*)realloc(ds->outData, ds->outLen + size);
        memcpy((void*) (ds->outData + ds->outLen), buf, size);
        ds->outLen += size;
    }

    return size;
}

/**
 * @brief Easylzma interface to perform compression
 *
 * @param[in] format: output format (0 for lzip format, 1 for lzma-alone format)
 * @param[in] inData: input stream buffer pointer
 * @param[in] inLen: input stream buffer length
 * @param[in] outData: output stream buffer pointer
 * @param[in] outLen: output stream buffer length
 * @param[in] level: positive number: use default compression level (5);
 *             negative interger: set compression level (-1, less, to -9, more compression)
 * @return return the fine grained lzma error code.
 */

int
simpleCompress(elzma_file_format format, const unsigned char* inData,
               size_t inLen, unsigned char** outData,
               size_t* outLen, int level) {
    int rc;
    elzma_compress_handle hand;

    /* allocate compression handle */
    hand = elzma_compress_alloc();
    assert(hand != NULL);

    rc = elzma_compress_config(hand, ELZMA_LC_DEFAULT,
                               ELZMA_LP_DEFAULT, ELZMA_PB_DEFAULT,
                               ((level > 0) ? 5 : -level), (1 << 20) /* 1mb */,
                               format, inLen);

    if (rc != ELZMA_E_OK) {
        elzma_compress_free(&hand);
        return rc;
    }

    /* now run the compression */
    {
        struct dataStream ds;
        ds.inData = inData;
        ds.inLen = inLen;
        ds.outData = NULL;
        ds.outLen = 0;

        rc = elzma_compress_run(hand, inputCallback, (void*) &ds,
                                outputCallback, (void*) &ds,
                                NULL, NULL);

        if (rc != ELZMA_E_OK) {
            if (ds.outData != NULL) {
                free(ds.outData);
            }

            elzma_compress_free(&hand);
            return rc;
        }

        *outData = ds.outData;
        *outLen = ds.outLen;
    }

    return rc;
}


/**
 * @brief Easylzma interface to perform decompression
 *
 * @param[in] format: output format (0 for lzip format, 1 for lzma-alone format)
 * @param[in] inData: input stream buffer pointer
 * @param[in] inLen: input stream buffer length
 * @param[in] outData: output stream buffer pointer
 * @param[in] outLen: output stream buffer length
 * @return return the fine grained lzma error code.
 */

int
simpleDecompress(elzma_file_format format, const unsigned char* inData,
                 size_t inLen, unsigned char** outData,
                 size_t* outLen) {
    int rc;
    elzma_decompress_handle hand;

    hand = elzma_decompress_alloc();

    /* now run the compression */
    {
        struct dataStream ds;
        ds.inData = inData;
        ds.inLen = inLen;
        ds.outData = NULL;
        ds.outLen = 0;

        rc = elzma_decompress_run(hand, inputCallback, (void*) &ds,
                                  outputCallback, (void*) &ds, format);

        if (rc != ELZMA_E_OK) {
            if (ds.outData != NULL) {
                free(ds.outData);
            }

            elzma_decompress_free(&hand);
            return rc;
        }

        *outData = ds.outData;
        *outLen = ds.outLen;
    }

    return rc;
}

#endif
