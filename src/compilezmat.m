if(~exist('OCTAVE_VERSION','builtin'))
    mex CFLAGS='$CFLAGS -O3 -g' -c lz4/lz4.c
    mex CFLAGS='$CFLAGS -O3 -g' -c lz4/lz4hc.c
    mex CFLAGS='$CFLAGS -O3 -g' -Ieasylzma/easylzma-0.0.8/include -c zmatlib.c
    mex zmat.cpp zmatlib.o lz4.o lz4hc.o easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -output ../zipmat -outdir ../ CXXLIBS='$CXXLIBS -lz'
else
    mex -O3 -g -c lz4/lz4.c
    mex -O3 -g -c lz4/lz4hc.c
    mex -Ieasylzma/easylzma-0.0.8/include -c zmatlib.c
    mex zmat.cpp zmatlib.o lz4.o lz4hc.o easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -o ../zipmat -lz
end

