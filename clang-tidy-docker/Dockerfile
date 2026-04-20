FROM ubuntu:25.10

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apt-get update
RUN apt-get upgrade -y && apt-get clean

RUN apt-get install -y wget software-properties-common g++-15 clang-21 clang-tidy-21 clang-format-21 lld-21 cmake make libssl-dev ninja-build libssl-dev libfuse3-dev fuse3 nfs-kernel-server rpcbind python3 linux-headers-generic git time && apt-get clean
RUN update-alternatives --install /usr/bin/lld lld /usr/bin/lld-21 10 \
 && update-alternatives --install /usr/bin/ld.lld ld.lld /usr/bin/ld.lld-21 10
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 10
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-21 10
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 10
RUN update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-21 10
RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-21 10


WORKDIR /build
ENTRYPOINT ["/bin/bash"]
