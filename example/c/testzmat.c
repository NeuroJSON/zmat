/***************************************************************************//**
**  \mainpage ZMat - A portable C-library and MATLAB/Octave toolbox for inline data compression
**
**  \author Qianqian Fang <q.fang at neu.edu>
**  \copyright Qianqian Fang, 2019,2020,2022
**
**  Demo on how to use zmatlib for simple compression and decompression in C
**
**  \section slicense License
**          GPL v3, see LICENSE.txt for details
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
  * to compile, one should add -I/path/to/zmatlib.h to the compilation command and
  * -L/path/to/libzmat.so(or .a) -lzmat -lz -lpthread to the linking command
  */

#include "zmatlib.h"

int main(void) {
    char* test[] = {"__o000o__(o)(o)__o000o__ =^_^=  __o000o__(o)(o)__o000o__"};

    int ret = 0, status = 0;
    size_t compressedlen, encodedlen, decodelen, decompressedlen;
    union TZMatFlags flags = {-9};

    /*output buffers will be allocated inside zmat functions, host is responsible to free after use*/
    unsigned char* compressed = NULL, *encoded = NULL, *decoded = NULL, *decompressed = NULL;

    /*=====================================*/
    /* compressing and encoding the string */
    /*=====================================*/

    /*first, perform zlib compression use the highest compression (-9); one can use zmat_encode as well*/
    ret = zmat_run(strlen(test[0]), (unsigned char*)test[0], &compressedlen, &compressed, zmZlib, &status, flags.iscompress);

    if (ret == 0) {
        /*next, encode the compressed data using base64*/
        ret = zmat_encode(compressedlen, compressed, &encodedlen, &encoded, zmBase64, &status);

        if (ret == 0) {
            printf("{\n\t\"original\":\"%s\",\n", test[0]);
            printf("\t\"encoded\":\"%s\",\n", encoded);
        }
    }

    /* error handling */
    if (ret) {
        printf("encoding failed, error code: %d: encoder error code %d\n", ret, status);

        if (compressed) {
            free(compressed);
        }

        return ret;
    }

    /*==========================================================*/
    /* decode and then decompress to restore the orginal string */
    /*==========================================================*/

    /*first, perform base64 decoding*/
    ret = zmat_decode(encodedlen, encoded, &decodelen, &decoded, zmBase64, &status);

    if (ret == 0) {
        /*next, decompress using zlib (deflate) */
        ret = zmat_decode(decodelen, decoded, &decompressedlen, &decompressed, zmZlib, &status);

        if (ret == 0) {
            printf("\t\"decompressed\":\"%s\",\n", decompressed);
        }
    }

    printf("}\n");

    /* error handling */
    if (ret) {
        printf("decoding failed, error code: %d: decoder error code %d\n", ret, status);

        if (decoded) {
            free(decoded);
        }

        return ret;
    }

    /*==================================*/
    /* host must free buffers after use */
    /*==================================*/

    if (compressed) {
        free(compressed);
    }

    if (encoded) {
        free(encoded);
    }

    if (decoded) {
        free(decoded);
    }

    if (decompressed) {
        free(decompressed);
    }

    return 0;
}
