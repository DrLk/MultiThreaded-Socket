FROM ubuntu:20.04

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apt-get update
RUN apt-get install -y wget software-properties-common

RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key
RUN apt-key add llvm-snapshot.gpg.key
RUN add-apt-repository 'deb http://apt.llvm.org/focal/ llvm-toolchain-focal-13 main'

RUN apt-get update
RUN apt-get install -y g++-10 clang-tidy-13 clang-format-13

WORKDIR /build
ENTRYPOINT ["/bin/bash"]
