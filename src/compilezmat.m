if(~exist('OCTAVE_VERSION','builtin'))
    mex zmat.cpp easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -outdir ../ CXXLIBS='$CXXLIBS -lz'
else
    mex zmat.cpp easylzma/easylzma-0.0.8/lib/libeasylzma_s.a -Ieasylzma/easylzma-0.0.8/include -o ../zmat
end

