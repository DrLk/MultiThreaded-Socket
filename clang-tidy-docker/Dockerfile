FROM ubuntu:23.04

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apt-get update
RUN apt-get install -y wget software-properties-common

#RUN wget -q https://apt.llvm.org/llvm-snapshot.gpg.key
#RUN apt-key add llvm-snapshot.gpg.key
#RUN add-apt-repository 'deb http://apt.llvm.org/lunar/ llvm-toolchain-lunar main'

RUN apt-get update
RUN apt-get upgrade
RUN apt-get install -y g++-13 clang-tidy-16 clang-format-16 cmake make
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13

WORKDIR /build
ENTRYPOINT ["/bin/bash"]
