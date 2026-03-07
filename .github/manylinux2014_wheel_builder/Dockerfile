FROM quay.io/pypa/manylinux2014_x86_64

# Install build dependencies
RUN yum install -y \
        gcc \
        gcc-c++ \
        make \
        pkgconfig \
    && yum clean all

# Copy build script
COPY build_wheels.sh /
RUN chmod +x /build_wheels.sh

WORKDIR /src
ENTRYPOINT ["/build_wheels.sh"]