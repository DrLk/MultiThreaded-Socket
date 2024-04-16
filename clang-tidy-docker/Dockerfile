FROM ubuntu:24.04

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apt-get update
RUN apt-get upgrade -y && apt-get clean

RUN apt-get install -y wget software-properties-common g++-14 clang-tidy-18 clang-format-18 cmake make libssl-dev ninja-build libssl-dev libfuse3-dev python3 linux-headers-generic git time && apt-get clean
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 10
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 10
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 10
RUN update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 10
RUN update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 10


WORKDIR /build
ENTRYPOINT ["/bin/bash"]
