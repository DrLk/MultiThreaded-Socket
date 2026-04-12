#!/usr/bin/env bash
# Run cmake, ninja, or tests inside Docker.
# Usage: build.sh generate|generate-release|build|test
set -euo pipefail

COMMAND="${1:-build}"

HOST_SRC=$(awk '/\/workspace /{print $4; exit}' /proc/self/mountinfo)
if [ -z "$HOST_SRC" ]; then
  HOST_SRC=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
fi

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
        CC=clang CXX=clang++ cmake -S /build/SimpleNetworkProtocol -B /build/SimpleNetworkProtocol/build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
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
    FILTER="${2:-}"
    FILTER_ARG=""
    if [ -n "$FILTER" ]; then
      FILTER_ARG="--gtest_filter=$FILTER"
    fi
    docker run --rm \
      -u 1000 \
      --device /dev/fuse \
      --cap-add SYS_ADMIN \
      --mount type=bind,src="$HOST_SRC",dst=/build \
      --mount type=bind,src=/usr/bin/fusermount3,dst=/usr/bin/fusermount3,readonly \
      clang-tidy-image -c "
        /build/SimpleNetworkProtocol/build/SimpleNetworkProtocolTest $FILTER_ARG
      "
    ;;
esac
