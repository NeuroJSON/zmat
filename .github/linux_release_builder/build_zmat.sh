#!/bin/bash
# Builds the Linux release mex files of zmat inside the manylinux2014 builder:
#
#   docker build -t zmat-linux-release-builder .github/linux_release_builder
#   docker run --rm -v $PWD:/src \
#       [-v $MATLAB_ROOT:$MATLAB_ROOT:ro -e MATLAB_ROOT=$MATLAB_ROOT] \
#       [-v /usr/include/octave-X.Y.Z:/octinc:ro -e OCTAVE_INCDIR=/octinc] \
#       zmat-linux-release-builder bash .github/linux_release_builder/build_zmat.sh
#
# MATLAB is the runner's (matlab-actions/setup-matlab), mounted read-only at
# the same path; the Octave headers are the runner's too.  Either may be left
# out to skip that mex file.  Every mex file is checked at the end: none may
# need a glibc symbol newer than 2.17, the oldest the image has, or link the
# GCC runtime libraries dynamically, or the build fails.

set -e
cd /src

# git refuses a checkout owned by another user, which a mounted one is
git config --global --add safe.directory '*' 2>/dev/null || true

export PATH=/src/.github/linux_release_builder:$PATH   # the mkoctfile stand-in
chmod +x /src/.github/linux_release_builder/mkoctfile

# the check at the end must only ever see what this run built
rm -f zipmat.mex zipmat.mexa64

if [ -n "$OCTAVE_INCDIR" ]; then
    echo "== zipmat for Octave (headers: $OCTAVE_INCDIR)"
    make -C src clean
    make -C src oct CC=gcc CXX=g++ USERLINKOPT="-s"
    # "make clean" removes every zipmat.mex*, so the Octave mex waits under another name
    mv zipmat.mex octave_zipmat.tmp
fi

if [ -n "$MATLAB_ROOT" ]; then
    echo "== zipmat for MATLAB ($MATLAB_ROOT)"
    make -C src clean
    make -C src mex CC=gcc CXX=g++ MEX="$MATLAB_ROOT/bin/mex"
    strip zipmat.mexa64
fi

[ -f octave_zipmat.tmp ] && mv octave_zipmat.tmp zipmat.mex

bash .github/linux_release_builder/check_glibc.sh zipmat.mex zipmat.mexa64
