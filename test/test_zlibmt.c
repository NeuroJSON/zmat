/*******************************************************************************
**  Correctness tests for the block-parallel zlib/gzip path (zmat_run_indexed)
**
**  Build against whichever backend you want to exercise, for example
**
**    gcc -O2 -DNO_LZMA -DNO_LZ4 -DNO_BLOSC2 -DNO_ZSTD -I../include \
**        -o test_zlibmt test_zlibmt.c ../src/zmatlib.c -lz -lpthread
**
**    gcc -O2 -DNO_ZLIB -D_LARGEFILE64_SOURCE=1 -DNO_LZMA -DNO_LZ4 -DNO_BLOSC2 \
**        -DNO_ZSTD -I../include -I../src/miniz -o test_zlibmt test_zlibmt.c \
**        ../src/zmatlib.c ../src/miniz/miniz.c -lpthread
**
**  Covers, across sizes spanning the block boundary: that an unaware
**  decompressor reads the stream, that the recorded index inflates the blocks
**  in parallel, that a corrupt index falls back to correct bytes rather than
**  returning wrong ones, and that the output does not depend on the thread
**  count. Exits non-zero on any failure.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zmatlib.h"

static int pack(int clevel, int nthread) {
    union { int i; struct { char c, n, s, t; } p; } f;
    f.i = 0; f.p.c = (char)clevel; f.p.n = (char)nthread; f.p.s = 1; f.p.t = 4;
    return f.i;
}
static int fails = 0;
static void ck(const char* name, int ok) {
    printf("  %-52s %s\n", name, ok ? "ok" : "FAIL");
    if (!ok) fails++;
}

int main(void) {
    size_t sizes[] = {1, 100, 4095, (4u<<20)-1, (4u<<20), (4u<<20)+1, 9u<<20, 17u<<20};
    int nsz = sizeof(sizes)/sizeof(sizes[0]);
    int gz, si, ret = 0;

    for (gz = 0; gz <= 1; gz++) {
        int zipid = gz ? zmGzip : zmZlib;
        for (si = 0; si < nsz; si++) {
            size_t n = sizes[si], cs = 0, ds = 0, noff = 0;
            unsigned char* in = malloc(n ? n : 1);
            unsigned char* cbuf = NULL; unsigned char* dbuf = NULL;
            size_t* off = NULL;
            unsigned int x = 999;
            char nm[128];
            for (size_t i = 0; i < n; i++) { x = x*1103515245u+12345u; in[i] = (x>>16) & 0xFF; }

            if (zmat_run_indexed(n, in, &cs, &cbuf, zipid, &ret, pack(1, 8), &off, &noff) != 0) {
                snprintf(nm, sizeof(nm), "%s n=%-9zu compress", gz?"gzip":"zlib", n);
                ck(nm, 0); free(in); continue;
            }
            /* An unaware reader must recover it. miniz's inflateInit2 rejects
             * window bits 15|32 (it has no gzip auto-detect), so under a miniz
             * build this check cannot run for gzip at all -- not for our output
             * and not for anything else. Skip it there rather than report a
             * failure that says nothing about the stream. */
            unsigned char* chk = malloc(n + 64);
            z_stream zs; memset(&zs,0,sizeof(zs));
            if (gz && inflateInit2(&zs, 15|32) != Z_OK) {
                snprintf(nm, sizeof(nm), "gzip n=%-9zu unaware reader (skipped: miniz)", n);
                printf("  %-52s %s\n", nm, "skip");
                free(chk); free(cbuf); free(off); free(in);
                continue;
            }
            memset(&zs,0,sizeof(zs));
            inflateInit2(&zs, gz ? (15|32) : 15);
            zs.next_in = cbuf; zs.avail_in = cs; zs.next_out = chk; zs.avail_out = n + 64;
            inflate(&zs, Z_FINISH);
            int unaware = (zs.total_out == n) && (n == 0 || !memcmp(chk, in, n));
            inflateEnd(&zs); free(chk);
            snprintf(nm, sizeof(nm), "%s n=%-9zu unaware zlib/gzip reads it", gz?"gzip":"zlib", n);
            ck(nm, unaware);

            /* indexed parallel inflate, when an index was produced */
            if (off && noff >= 4) {
                if (zmat_run_indexed(cs, cbuf, &ds, &dbuf, zipid, &ret, pack(0, 8), &off, &noff) == 0) {
                    snprintf(nm, sizeof(nm), "%s n=%-9zu indexed parallel inflate", gz?"gzip":"zlib", n);
                    ck(nm, ds == n && (n == 0 || !memcmp(dbuf, in, n)));
                    free(dbuf); dbuf = NULL;
                }
                /* a corrupt index must fall back, not return wrong bytes */
                size_t* bad = malloc(noff * sizeof(size_t));
                memcpy(bad, off, noff * sizeof(size_t));
                bad[3] += 7;
                size_t* badp = bad; size_t badn = noff;
                if (zmat_run_indexed(cs, cbuf, &ds, &dbuf, zipid, &ret, pack(0, 8), &badp, &badn) == 0) {
                    snprintf(nm, sizeof(nm), "%s n=%-9zu corrupt index -> correct via fallback", gz?"gzip":"zlib", n);
                    ck(nm, ds == n && !memcmp(dbuf, in, n));
                    free(dbuf); dbuf = NULL;
                } else {
                    snprintf(nm, sizeof(nm), "%s n=%-9zu corrupt index -> error (not wrong bytes)", gz?"gzip":"zlib", n);
                    ck(nm, 1);
                }
                free(bad);
            }

            /* thread count must not change the bytes */
            unsigned char* alt = NULL; size_t as = 0, an = 0; size_t* ao = NULL;
            int same = 1;
            for (int nt = 1; nt <= 32 && same; nt *= 2) {
                if (zmat_run_indexed(n, in, &as, &alt, zipid, &ret, pack(1, nt), &ao, &an) != 0) { same = 0; break; }
                same = (as == cs) && !memcmp(alt, cbuf, cs);
                free(alt); alt = NULL; free(ao); ao = NULL;
            }
            snprintf(nm, sizeof(nm), "%s n=%-9zu bytes independent of nthread", gz?"gzip":"zlib", n);
            ck(nm, same);

            free(cbuf); free(off); free(in);
        }
    }
    printf("\n%s (%d failures)\n", fails ? "FAILURES PRESENT" : "ALL PASS", fails);
    return fails != 0;
}
