FROM alpine:edge

LABEL org.opencontainers.image.source https://github.com/DrLk/MultiThreaded-Socket

RUN apk update
RUN apk upgrade

RUN apk add --no-cache python3 g++ clang16-extra-tools cmake make openssl-dev include-what-you-use

WORKDIR /build
ENTRYPOINT ["/bin/sh"]
