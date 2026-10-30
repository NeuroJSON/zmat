# Release builder for the Linux MATLAB/Octave mex files of zmat.
#
# manylinux2014 is CentOS 7, glibc 2.17: anything linked here needs at most
# GLIBC_2.17 and runs on any distribution newer than that -- Ubuntu 14.04 on.
# Built on the GitHub ubuntu-22.04 runner instead, the mex files picked up
# GLIBC_2.34 symbols (pthread moved into libc in 2.34) and would not load on
# Ubuntu 20.04 or older.
#
# glibc itself stays dynamic: a mex file has to share MATLAB's or Octave's
# libc. libstdc++ and libgcc are linked statically by the build script.
#
# Run with "docker run", not as a job container: the JavaScript actions
# (checkout, upload-artifact) need node 20, which needs glibc 2.28.

FROM quay.io/pypa/manylinux2014_x86_64

RUN yum install -y devtoolset-8-gcc devtoolset-8-gcc-c++ which make zip \
    && yum clean all

ENV PATH=/opt/rh/devtoolset-8/root/usr/bin:${PATH}

WORKDIR /src
