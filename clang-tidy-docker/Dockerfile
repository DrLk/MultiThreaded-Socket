FROM alpine:edge

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apk update
RUN apk upgrade

RUN apk add --no-cache linux-headers python3 git g++ clang17 clang17-extra-tools cmake make ninja openssl-dev include-what-you-use fuse3-dev

WORKDIR /build
ENTRYPOINT ["/bin/sh"]
