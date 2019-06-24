if(~exist('OCTAVE_VERSION','builtin'))
    mex CFLAGS='$CFLAGS -O3 -g' -Ieasylzma/easylzma-0.0.8/include -c zmatlib.c
    mex zmat.cpp zmatlib.o easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -outdir ../ CXXLIBS='$CXXLIBS -lz'
else
    mex -Ieasylzma/easylzma-0.0.8/include -c zmatlib.c
    mex zmat.cpp zmatlib.o easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -o ../zmat
end

