if(~exist('OCTAVE_VERSION','builtin'))
    mex zmat.cpp -outdir ../ CXXLIBS='$CXXLIBS -lz'
else
    mex zmat.cpp -o ../zmat
end

