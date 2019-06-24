#ifndef ZMAT_LIB_H
#define ZMAT_LIB_H

#ifndef NO_LZMA
  #include "easylzma/compress.h"
  #include "easylzma/decompress.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum TZipMethod {zmZlib, zmGzip, zmBase64, zmLzip, zmLzma};

int zmat_run(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret, const int iscompress);
int zmat_encode(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret);
int zmat_decode(const size_t inputsize, unsigned char *inputstr, size_t *outputsize, unsigned char **outputbuf, const int zipid, int *ret);

int  zmat_keylookup(char *origkey, const char *table[]);
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
                   size_t * outLen);

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