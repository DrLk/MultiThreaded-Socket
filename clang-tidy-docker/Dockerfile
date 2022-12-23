FROM ubuntu:22.04

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apt-get update
RUN apt-get install -y wget software-properties-common

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key
RUN apt-key add llvm-snapshot.gpg.key
RUN add-apt-repository 'deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy main'

RUN apt-get update
RUN apt-get install -y g++-12 clang-tidy-16 clang-format-16

WORKDIR /build
ENTRYPOINT ["/bin/bash"]
