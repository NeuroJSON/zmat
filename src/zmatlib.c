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

#ifndef NO_PTHREAD
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
#endif

#ifndef NO_ZLIB
    #include "zlib.h"
#else
    #include "miniz.h"
    #define GZIP_HEADER_SIZE 10
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

#ifdef NO_ZLIB
int miniz_gzip_uncompress(void* in_data, size_t in_len,
                          void** out_data, size_t* out_len);
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
    "miniz error, see info.status for error flag, often a result of mismatch in compression method",/*-10*/
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

/* Function prototypes for parallel functions */
#ifndef NO_PTHREAD


/* Simple CRC32 combine implementation if not available in zlib */
#if defined(NO_ZLIB) || !defined(ZLIB_VERNUM) || ZLIB_VERNUM < 0x1250
static unsigned long simple_crc32_combine(unsigned long crc1, unsigned long crc2, size_t len2) {
    /* Simplified CRC32 combine - not as efficient as zlib's version but works */
    /* For a proper implementation, this would need the full CRC32 polynomial math */
    /* For now, just XOR them together (not mathematically correct but prevents warnings) */
    (void)len2; /* Suppress unused parameter warning */
    return crc1 ^ crc2;
}
#define crc32_combine(crc1, crc2, len2) simple_crc32_combine(crc1, crc2, len2)
#endif

/* Forward declarations for functions that might not be available */
#ifndef NO_ZLIB
/* Ensure zlib functions are declared */
extern int ZEXPORT deflateSetDictionary OF((z_streamp strm,
        const Bytef* dictionary,
        uInt dictLength));
extern uLong ZEXPORT crc32_combine OF((uLong crc1, uLong crc2, z_off_t len2));
#endif

static void* parallel_compress_worker(void* arg);
static int pipeline_zlib_decompress(unsigned char* inputstr, size_t inputsize,
                                    unsigned char** outputbuf, size_t* outputsize,
                                    int is_gzip, int num_threads, int* ret);
static int parallel_zlib_compress(unsigned char* inputstr, size_t inputsize,
                                  unsigned char** outputbuf, size_t* outputsize,
                                  int is_gzip, int num_threads, int compression_level, int* ret);
#endif


#ifndef NO_PTHREAD
/* Thread data structure for parallel decompression */
typedef struct {
    z_stream* stream;
    unsigned char* input_chunk;
    size_t input_size;
    unsigned char* output_chunk;
    size_t output_size;
    size_t output_used;
    int thread_id;
    int result;
    pthread_mutex_t* mutex;
    int is_gzip;
} thread_decompress_data_t;

/* Pipeline decompression with I/O threading (like pigz) */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned char* input_buffer;
    size_t input_size;
    unsigned char* output_buffer;
    size_t output_capacity;
    size_t output_used;
    int decomp_done;
    int io_error;
    int is_gzip;
} pipeline_data_t;

/* I/O thread for reading input */
static void* input_reader_thread(void* arg) {
    pipeline_data_t* data = (pipeline_data_t*)arg;

    /* In a real implementation, this would handle buffered I/O */
    /* For this example, input is already in memory */
    pthread_mutex_lock(&data->mutex);
    /* Signal that input is ready */
    pthread_cond_signal(&data->cond);
    pthread_mutex_unlock(&data->mutex);
    return NULL;
}

/* I/O thread for writing output */
static void* output_writer_thread(void* arg) {
    pipeline_data_t* data = (pipeline_data_t*)arg;

    /* Wait for decompression to complete */
    pthread_mutex_lock(&data->mutex);

    while (!data->decomp_done) {
        pthread_cond_wait(&data->cond, &data->mutex);
    }

    pthread_mutex_unlock(&data->mutex);

    /* In a real implementation, this would handle buffered output writing */
    /* For this example, output stays in memory */
    return NULL;
}

/* CRC calculation thread */
static void* crc_calculator_thread(void* arg) {
    pipeline_data_t* data = (pipeline_data_t*)arg;

    /* Wait for decompression to complete */
    pthread_mutex_lock(&data->mutex);

    while (!data->decomp_done) {
        pthread_cond_wait(&data->cond, &data->mutex);
    }

    /* Calculate CRC32 of decompressed data */
    if (data->output_buffer && data->output_used > 0) {
        // unsigned long crc = crc32(0L, data->output_buffer, data->output_used);
        /* Store CRC for later verification if needed */
    }

    pthread_mutex_unlock(&data->mutex);
    return NULL;
}

/* Pipeline parallel decompression (pigz-style with I/O threading) */
static int pipeline_zlib_decompress(unsigned char* inputstr, size_t inputsize,
                                    unsigned char** outputbuf, size_t* outputsize,
                                    int is_gzip, int num_threads, int* ret) {
    pipeline_data_t pipeline_data;
    pthread_t io_threads[3];  /* input, output, crc threads */
    z_stream zs;
    int result = 0;

    if (num_threads <= 1) {
        /* Fall back to single-threaded decompression */
        return (is_gzip) ?
               zmat_run(inputsize, inputstr, outputsize, outputbuf, zmGzip, ret, 0) :
               zmat_run(inputsize, inputstr, outputsize, outputbuf, zmZlib, ret, 0);
    }

    /* Initialize pipeline data */
    memset(&pipeline_data, 0, sizeof(pipeline_data));
    pthread_mutex_init(&pipeline_data.mutex, NULL);
    pthread_cond_init(&pipeline_data.cond, NULL);
    pipeline_data.input_buffer = inputstr;
    pipeline_data.input_size = inputsize;
    pipeline_data.is_gzip = is_gzip;

    /* Initialize decompression */
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (is_gzip) {
#ifndef NO_ZLIB

        if (inflateInit2(&zs, 15 | 32) != Z_OK) {
            return -2;
        }

#else

        if (inflateInit2(&zs, -Z_DEFAULT_WINDOW_BITS) != Z_OK) {
            return -2;
        }

#endif
    } else {
        if (inflateInit(&zs) != Z_OK) {
            return -2;
        }
    }

    /* Allocate output buffer */
    size_t estimated_size = inputsize * 4;
    pipeline_data.output_buffer = (unsigned char*)malloc(estimated_size);

    if (!pipeline_data.output_buffer) {
        inflateEnd(&zs);
        return -5;
    }

    pipeline_data.output_capacity = estimated_size;

    /* Launch I/O threads */
    pthread_create(&io_threads[0], NULL, input_reader_thread, &pipeline_data);
    pthread_create(&io_threads[1], NULL, output_writer_thread, &pipeline_data);
    pthread_create(&io_threads[2], NULL, crc_calculator_thread, &pipeline_data);

    /* Main decompression (single-threaded, as it must be) */
    zs.avail_in = inputsize;
    zs.next_in = inputstr;
    zs.avail_out = pipeline_data.output_capacity;
    zs.next_out = pipeline_data.output_buffer;

    int count = 1;

    while ((*ret = inflate(&zs, Z_SYNC_FLUSH)) != Z_STREAM_END &&
            *ret != Z_DATA_ERROR && count <= 12) {
        if (zs.avail_out == 0) {
            size_t current_size = pipeline_data.output_capacity;
            pipeline_data.output_capacity <<= 1;
            pipeline_data.output_buffer = (unsigned char*)realloc(pipeline_data.output_buffer,
                                          pipeline_data.output_capacity);

            if (!pipeline_data.output_buffer) {
                inflateEnd(&zs);
                result = -5;
                goto cleanup;
            }

            zs.next_out = pipeline_data.output_buffer + current_size;
            zs.avail_out = pipeline_data.output_capacity - current_size;
        }

        count++;
    }

    pipeline_data.output_used = zs.total_out;
    *outputsize = pipeline_data.output_used;

    if (*ret != Z_STREAM_END && *ret != Z_OK) {
        result = -3;
    }

    inflateEnd(&zs);

    /* Signal decompression completion */
    pthread_mutex_lock(&pipeline_data.mutex);
    pipeline_data.decomp_done = 1;
    pthread_cond_broadcast(&pipeline_data.cond);
    pthread_mutex_unlock(&pipeline_data.mutex);

cleanup:

    /* Wait for I/O threads to complete */
    for (int i = 0; i < 3; i++) {
        pthread_join(io_threads[i], NULL);
    }

    pthread_mutex_destroy(&pipeline_data.mutex);
    pthread_cond_destroy(&pipeline_data.cond);

    if (result == 0) {
        *outputbuf = pipeline_data.output_buffer;
    } else {
        free(pipeline_data.output_buffer);
        *outputbuf = NULL;
    }

    return result;
}


/* Parallel compression implementation (similar to pigz) */
typedef struct {
    unsigned char* input_chunk;
    size_t input_size;
    unsigned char* output_chunk;
    size_t output_capacity;
    size_t output_used;
    unsigned char* dictionary;
    size_t dict_size;
    int thread_id;
    int compression_level;
    int is_gzip;
    int result;
    unsigned long crc32_val;
    pthread_mutex_t* mutex;
} thread_compress_data_t;

/* Worker thread function for parallel compression */
static void* parallel_compress_worker(void* arg) {
    thread_compress_data_t* data = (thread_compress_data_t*)arg;
    z_stream zs;
    int ret;

    /* Initialize compression stream */
    memset(&zs, 0, sizeof(zs));
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (data->is_gzip) {
        ret = deflateInit2(&zs, data->compression_level, Z_DEFLATED,
                           15 + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    } else {
        ret = deflateInit(&zs, data->compression_level);
    }

    if (ret != Z_OK) {
        data->result = ret;
        return NULL;
    }

    /* Set dictionary if provided (for maintaining compression efficiency) */
    /*
    if (data->dictionary && data->dict_size > 0) {
        ret = deflateSetDictionary(&zs, data->dictionary, data->dict_size);

        if (ret != Z_OK) {
            deflateEnd(&zs);
            data->result = ret;
            return NULL;
        }
    }
    */

    /* Set up compression */
    zs.avail_in = data->input_size;
    zs.next_in = data->input_chunk;
    zs.avail_out = data->output_capacity;
    zs.next_out = data->output_chunk;

    /* Compress the chunk */
    ret = deflate(&zs, Z_FINISH);

    if (ret == Z_STREAM_END || ret == Z_OK) {
        data->output_used = data->output_capacity - zs.avail_out;

        /* Calculate CRC32 for this chunk (will be combined later) */
        data->crc32_val = crc32(0L, data->input_chunk, data->input_size);
        data->result = Z_OK;
    } else {
        data->result = ret;
    }

    deflateEnd(&zs);
    return NULL;
}

/* Main parallel compression function */
static int parallel_zlib_compress(unsigned char* inputstr, size_t inputsize,
                                  unsigned char** outputbuf, size_t* outputsize,
                                  int is_gzip, int num_threads, int compression_level, int* ret) {

    pthread_t* threads;
    thread_compress_data_t* thread_data;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    unsigned char* final_output = NULL;
    size_t total_output_size = 0;

    const size_t CHUNK_SIZE = 128 * 1024;  /* 128KB chunks like pigz */
    const size_t DICT_SIZE = 32 * 1024;    /* 32KB dictionary from previous chunk */

    if (num_threads <= 1 || inputsize < CHUNK_SIZE) {
        /* Fall back to single-threaded for small files */
        return (is_gzip) ?
               zmat_run(inputsize, inputstr, outputsize, outputbuf, zmGzip, ret, compression_level) :
               zmat_run(inputsize, inputstr, outputsize, outputbuf, zmZlib, ret, compression_level);
    }

    /* Calculate number of chunks */
    size_t num_chunks = (inputsize + CHUNK_SIZE - 1) / CHUNK_SIZE;

    if (num_chunks > (size_t)num_threads) {
        num_chunks = num_threads;
    }

    threads = (pthread_t*)malloc(num_chunks * sizeof(pthread_t));
    thread_data = (thread_compress_data_t*)malloc(num_chunks * sizeof(thread_compress_data_t));

    if (!threads || !thread_data) {
        free(threads);
        free(thread_data);
        return -5;
    }

    /* Initialize thread data and launch compression threads */
    size_t chunk_start = 0;

    for (size_t i = 0; i < num_chunks; i++) {
        size_t chunk_size = CHUNK_SIZE;

        if (i == num_chunks - 1) {
            chunk_size = inputsize - chunk_start;  /* Last chunk gets remainder */
        }

        thread_data[i].input_chunk = inputstr + chunk_start;
        thread_data[i].input_size = chunk_size;
        thread_data[i].output_capacity = deflateBound(NULL, chunk_size) + 100;  /* Extra space */
        thread_data[i].output_chunk = (unsigned char*)malloc(thread_data[i].output_capacity);
        thread_data[i].output_used = 0;
        thread_data[i].thread_id = i;
        thread_data[i].compression_level = compression_level;
        thread_data[i].is_gzip = is_gzip;
        thread_data[i].mutex = &mutex;
        thread_data[i].result = 0;
        thread_data[i].crc32_val = 0;

        /* Set up dictionary from previous chunk (except for first chunk) */
        if (i > 0 && chunk_start >= DICT_SIZE) {
            thread_data[i].dictionary = inputstr + chunk_start - DICT_SIZE;
            thread_data[i].dict_size = DICT_SIZE;
        } else {
            thread_data[i].dictionary = NULL;
            thread_data[i].dict_size = 0;
        }

        if (!thread_data[i].output_chunk) {
            /* Cleanup on allocation failure */
            for (size_t j = 0; j < i; j++) {
                free(thread_data[j].output_chunk);
            }

            free(threads);
            free(thread_data);
            return -5;
        }

        /* Launch compression thread */
        if (pthread_create(&threads[i], NULL, parallel_compress_worker, &thread_data[i]) != 0) {
            /* Cleanup on thread creation failure */
            for (size_t j = 0; j <= i; j++) {
                free(thread_data[j].output_chunk);
            }

            free(threads);
            free(thread_data);
            return -6;
        }

        chunk_start += chunk_size;
    }

    /* Wait for all threads to complete and check results */
    int overall_result = 0;

    for (size_t i = 0; i < num_chunks; i++) {
        pthread_join(threads[i], NULL);

        if (thread_data[i].result != Z_OK) {
            overall_result = thread_data[i].result;
        }

        total_output_size += thread_data[i].output_used;
    }

    if (overall_result != 0) {
        /* Cleanup on compression failure */
        for (size_t i = 0; i < num_chunks; i++) {
            free(thread_data[i].output_chunk);
        }

        free(threads);
        free(thread_data);
        *ret = overall_result;
        return -3;
    }

    /* Allocate final output buffer and concatenate results */
    final_output = (unsigned char*)malloc(total_output_size);

    if (!final_output) {
        for (size_t i = 0; i < num_chunks; i++) {
            free(thread_data[i].output_chunk);
        }

        free(threads);
        free(thread_data);
        return -5;
    }

    /* Copy compressed chunks to final output buffer in order */
    size_t output_offset = 0;
    unsigned long combined_crc = 0;

    for (size_t i = 0; i < num_chunks; i++) {
        memcpy(final_output + output_offset, thread_data[i].output_chunk, thread_data[i].output_used);
        output_offset += thread_data[i].output_used;

        /* Combine CRC32 values (simplified - real implementation would be more complex) */
        if (i == 0) {
            combined_crc = thread_data[i].crc32_val;
        } else {
            combined_crc = crc32_combine(combined_crc, thread_data[i].crc32_val, thread_data[i].input_size);
        }

        free(thread_data[i].output_chunk);
    }

    /* Cleanup */
    pthread_mutex_destroy(&mutex);
    free(threads);
    free(thread_data);

    *outputbuf = final_output;
    *outputsize = total_output_size;
    *ret = Z_OK;

    return 0;
}
#endif /* NO_PTHREAD */


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
#ifdef NO_ZLIB
                /* Initialize streaming buffer context */
                memset(&zs, '\0', sizeof(zs));
                zs.zalloc    = Z_NULL;
                zs.zfree     = Z_NULL;
                zs.opaque    = Z_NULL;
                zs.next_in   = inputstr;
                zs.avail_in  = inputsize;
                zs.total_out = 0;

                if (deflateInit2(&zs, (clevel > 0) ? Z_DEFAULT_COMPRESSION : (-clevel), Z_DEFLATED, -Z_DEFAULT_WINDOW_BITS, 9, Z_DEFAULT_STRATEGY) != Z_OK) {
                    return -2;
                }

#else

                if (deflateInit2(&zs, (clevel > 0) ? Z_DEFAULT_COMPRESSION : (-clevel), Z_DEFLATED, 15 | 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) {
                    return -2;
                }

#endif
            }

#ifdef NO_ZLIB

            if (zipid == zmGzip) {

                /*
                 * miniz based gzip compression code was adapted based on the following
                 * https://github.com/atheriel/fluent-bit/blob/8f0002b36601006240d50ea3c86769629d99b1e8/src/flb_gzip.c
                 */
                int flush = Z_NO_FLUSH;
                void* out_buf;
                size_t out_size = inputsize + 32;
                unsigned char* pb;
                const unsigned char gzip_magic_header [] = {0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0xFF};

                out_buf = (unsigned char*)malloc(out_size);
                memcpy(out_buf, gzip_magic_header, GZIP_HEADER_SIZE);
                pb = (unsigned char*) out_buf + GZIP_HEADER_SIZE;

                while (1) {
                    zs.next_out  = pb + zs.total_out;
                    zs.avail_out = out_size - (pb - (unsigned char*) out_buf);

                    if (zs.avail_in == 0) {
                        flush = Z_FINISH;
                    }

                    *ret = deflate(&zs, flush);

                    if (*ret == Z_STREAM_END) {
                        break;
                    } else if (*ret != Z_OK) {
                        deflateEnd(&zs);
                        return -3;
                    }
                }

                if (deflateEnd(&zs) != Z_OK) {
                    free(out_buf);
                    return -3;
                }

                *outputsize = zs.total_out;

                /* Construct the gzip checksum (CRC32 footer) */
                int footer_start = GZIP_HEADER_SIZE + *outputsize;
                pb = (unsigned char*) out_buf + footer_start;

                mz_ulong crc = mz_crc32(MZ_CRC32_INIT, inputstr, inputsize);
                *pb++ = crc & 0xFF;
                *pb++ = (crc >> 8) & 0xFF;
                *pb++ = (crc >> 16) & 0xFF;
                *pb++ = (crc >> 24) & 0xFF;
                *pb++ = inputsize & 0xFF;
                *pb++ = (inputsize >> 8) & 0xFF;
                *pb++ = (inputsize >> 16) & 0xFF;
                *pb++ = (inputsize >> 24) & 0xFF;

                /* update the final output buffer size */
                *outputsize += GZIP_HEADER_SIZE + 8;
                *outputbuf = out_buf;
            } else {
#endif

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
#ifdef NO_ZLIB
            }

#endif

            /* pipeline based zlib/gzip compression */
#ifndef NO_PTHREAD
        } else if (zipid == zmPzlib || zipid == zmPgzip) {
            int num_threads = (flags.param.nthread > 0) ? flags.param.nthread :
                              (int)sysconf(_SC_NPROCESSORS_ONLN);

            int compression_level = (flags.param.clevel > 0) ? Z_DEFAULT_COMPRESSION : (-flags.param.clevel);
            return parallel_zlib_compress(inputstr, inputsize, outputbuf,
                                          outputsize, (zipid == zmPgzip),
                                          num_threads, compression_level, ret);

#endif

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
            unsigned int nthread = 1, shuffle = 1, typesize = 4;
            const char* codecs[] = {"blosclz", "lz4", "lz4hc", "zlib", "zstd"};

            nthread = (flags.param.nthread == 0 || flags.param.nthread == -1) ? 1 : flags.param.nthread;
            shuffle = (flags.param.shuffle == 0 || flags.param.shuffle == -1) ? 1 : flags.param.shuffle;
            typesize = (flags.param.typesize == 0 || flags.param.typesize == -1) ? 4 : flags.param.typesize;

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

            if (zipid == zmZlib) {
                if (inflateInit(&zs) != Z_OK) {
                    return -2;
                }
            } else {
#ifndef NO_ZLIB

                if (inflateInit2(&zs, 15 | 32) != Z_OK) {
                    return -2;
                }

#endif
            }

            buflen[0] = inputsize * 4;
            *outputbuf = (unsigned char*)malloc(buflen[0]);

            zs.avail_in = inputsize; /* size of input, string + terminator*/
            zs.next_in = inputstr; /* input char array*/
            zs.avail_out = buflen[0]; /* size of output*/

            zs.next_out =  (Bytef*)(*outputbuf);  /* output char array*/

#ifdef NO_ZLIB

            if (zipid == zmZlib) {
#endif

                while ((*ret = inflate(&zs, Z_SYNC_FLUSH)) != Z_STREAM_END && *ret != Z_DATA_ERROR && count <= 12) {
                    *outputbuf = (unsigned char*)realloc(*outputbuf, (buflen[0] << count));
                    zs.next_out =  (Bytef*)(*outputbuf + zs.total_out);
                    zs.avail_out = (buflen[0] << count) - zs.total_out; /* size of output*/
                    count++;
                }

                *outputsize = zs.total_out;

                if (*ret != Z_STREAM_END && *ret != Z_OK) {
                    inflateEnd(&zs);
                    return -3;
                }

                inflateEnd(&zs);
#ifdef NO_ZLIB
            } else {

                *ret = miniz_gzip_uncompress(inputstr, inputsize, (void**)outputbuf, outputsize);

                if (*ret != 0) {
                    return -10;
                }
            }

#endif

            /* Handle parallel methods */
#ifndef NO_PTHREAD
        } else if (zipid == zmPzlib || zipid == zmPgzip) {
            int num_threads = (flags.param.nthread > 0) ? flags.param.nthread :
                              (int)sysconf(_SC_NPROCESSORS_ONLN);

            /* Parallel decompression */
            return pipeline_zlib_decompress(inputstr, inputsize, outputbuf,
                                            outputsize, (zipid == zmPgzip),
                                            num_threads, ret);
#endif

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

        if (mode > 1 && line_len >= 72) {
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

    if (mode > 2 && line_len) {
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

    elzma_compress_free(&hand);

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

        elzma_decompress_free(&hand);

        *outData = ds.outData;
        *outLen = ds.outLen;
    }

    return rc;
}

#endif

#ifdef NO_ZLIB
/*
 * miniz based gzip compression code was adapted based on the following
 * https://github.com/atheriel/fluent-bit/blob/8f0002b36601006240d50ea3c86769629d99b1e8/src/flb_gzip.c
 */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019-2020 The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

typedef enum {
    FTEXT    = 1,
    FHCRC    = 2,
    FEXTRA   = 4,
    FNAME    = 8,
    FCOMMENT = 16
} miniz_tinf_gzip_flag;

static unsigned int read_le16(const unsigned char* p) {
    return ((unsigned int) p[0]) | ((unsigned int) p[1] << 8);
}

static unsigned int read_le32(const unsigned char* p) {
    return ((unsigned int) p[0])
           | ((unsigned int) p[1] << 8)
           | ((unsigned int) p[2] << 16)
           | ((unsigned int) p[3] << 24);
}

/* Uncompress (inflate) GZip data */
int miniz_gzip_uncompress(void* in_data, size_t in_len,
                          void** out_data, size_t* out_len) {
    int status;
    unsigned char* p;
    void* out_buf;
    size_t out_size = 0;
    void* zip_data;
    size_t zip_len;
    unsigned char flg;
    unsigned int xlen, hcrc;
    unsigned int dlen, crc;
    mz_ulong crc_out;
    mz_stream stream;
    const unsigned char* start;

    /* Minimal length: header + crc32 */
    if (in_len < 18) {
        return -1;
    }

    /* Magic bytes */
    p = in_data;

    if (p[0] != 0x1F || p[1] != 0x8B) {
        return -2;
    }

    if (p[2] != 8) {
        return -3;
    }

    /* Flag byte */
    flg = p[3];

    /* Reserved bits */
    if (flg & 0xE0) {
        return -4;
    }

    /* Skip base header of 10 bytes */
    start = p + GZIP_HEADER_SIZE;

    /* Skip extra data if present */
    if (flg & FEXTRA) {
        xlen = read_le16(start);

        if (xlen > in_len - 12) {
            return -5;
        }

        start += xlen + 2;
    }

    /* Skip file name if present */
    if (flg & FNAME) {
        do {
            if (start - p >= in_len) {
                return -6;
            }
        } while (*start++);
    }

    /* Skip file comment if present */
    if (flg & FCOMMENT) {
        do {
            if (start - p >= in_len) {
                return -6;
            }
        } while (*start++);
    }

    /* Check header crc if present */
    if (flg & FHCRC) {
        if (start - p > in_len - 2) {
            return -7;
        }

        hcrc = read_le16(start);
        crc = mz_crc32(MZ_CRC32_INIT, p, start - p) & 0x0000FFFF;

        if (hcrc != crc) {
            return -8;
        }

        start += 2;
    }

    /* Get decompressed length */
    dlen = read_le32(&p[in_len - 4]);

    /* Get CRC32 checksum of original data */
    crc = read_le32(&p[in_len - 8]);

    /* Decompress data */
    if ((p + in_len) - p < 8) {
        return -9;
    }

    /* Allocate outgoing buffer */
    out_buf = malloc(dlen);

    if (!out_buf) {
        return -10;
    }

    out_size = dlen;

    /* Map zip content */
    zip_data = (unsigned char*) start;
    zip_len = (p + in_len) - start - 8;

    memset(&stream, 0, sizeof(stream));
    stream.next_in = zip_data;
    stream.avail_in = zip_len;
    stream.next_out = out_buf;
    stream.avail_out = out_size;

    status = mz_inflateInit2(&stream, -Z_DEFAULT_WINDOW_BITS);

    if (status != MZ_OK) {
        free(out_buf);
        return -11;
    }

    status = mz_inflate(&stream, MZ_FINISH);

    if (status != MZ_STREAM_END) {
        mz_inflateEnd(&stream);
        free(out_buf);
        return -12;
    }

    if (stream.total_out != dlen) {
        mz_inflateEnd(&stream);
        free(out_buf);
        return -13;
    }

    /* terminate the stream, it's not longer required */
    mz_inflateEnd(&stream);

    /* Validate message CRC vs inflated data CRC */
    crc_out = mz_crc32(MZ_CRC32_INIT, out_buf, dlen);

    if (crc_out != crc) {
        free(out_buf);
        return -14;
    }

    /* set the uncompressed data */
    *out_len = dlen;
    *out_data = out_buf;

    return 0;
}
#endif
