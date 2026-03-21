#!/usr/bin/env bash
# Run cmake, ninja, or tests inside Docker.
# Usage: build.sh generate|generate-release|build|test
set -euo pipefail

COMMAND="${1:-build}"

HOST_SRC=$(awk '/\/workspace /{print $4; exit}' /proc/self/mountinfo)

case "$COMMAND" in
  generate)
    docker run --rm \
      -u 1000 \
      --mount type=bind,src="$HOST_SRC",dst=/build \
      clang-tidy-image -c "
        CC=clang CXX=clang++ cmake -S /build/SimpleNetworkProtocol -B /build/SimpleNetworkProtocol/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
      "
    ;;
  generate-release)
    docker run --rm \
      -u 1000 \
      --mount type=bind,src="$HOST_SRC",dst=/build \
      clang-tidy-image -c "
        CC=clang CXX=clang++ cmake -S /build/SimpleNetworkProtocol -B /build/SimpleNetworkProtocol/build -G Ninja -DCMAKE_BUILD_TYPE=Release
      "
    ;;
  build)
    docker run --rm \
      -u 1000 \
      --mount type=bind,src="$HOST_SRC",dst=/build \
      clang-tidy-image -c "
        cmake --build /build/SimpleNetworkProtocol/build
      "
    ;;
  test)
    docker run --rm \
      -u 1000 \
      --mount type=bind,src="$HOST_SRC",dst=/build \
      clang-tidy-image -c "
        /build/SimpleNetworkProtocol/build/SimpleNetworkProtocolTest
      "
    ;;
esac
