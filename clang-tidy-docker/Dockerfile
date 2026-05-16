FROM fedora:44

LABEL org.opencontainers.image.source=https://github.com/DrLk/MultiThreaded-Socket

RUN dnf upgrade -y && dnf clean all

RUN dnf install -y \
    wget \
    gcc \
    gcc-c++ \
    libstdc++-static \
    libtsan \
    libasan \
    libubsan \
    clang \
    clang-tools-extra \
    lld \
    cmake \
    make \
    openssl-devel \
    ninja-build \
    fuse3-devel \
    fuse3 \
    nfs-utils \
    rpcbind \
    python3 \
    kernel-headers \
    git \
    time \
    && dnf clean all


WORKDIR /build
ENTRYPOINT ["/bin/bash"]
