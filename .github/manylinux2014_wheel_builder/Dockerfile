FROM quay.io/pypa/manylinux2014_x86_64

# Install build dependencies
# libgomp-static is needed when STATIC=1 is used for Makefile-based builds;
# it also ensures libgomp.a is present for the linker.
RUN yum install -y \
        gcc \
        gcc-c++ \
        make \
        pkgconfig \
        libgomp \
        libgomp-static \
    && yum clean all

# Copy build script
COPY build_wheels.sh /
RUN chmod +x /build_wheels.sh

WORKDIR /src
ENTRYPOINT ["/build_wheels.sh"]