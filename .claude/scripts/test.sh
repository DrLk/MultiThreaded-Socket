#!/usr/bin/env bash
# Run SimpleNetworkProtocolTest inside Docker.
# Usage: test.sh [gtest_filter]
set -euo pipefail

FILTER_ARG=""
if [ -n "${1:-}" ]; then
  FILTER_ARG="--gtest_filter=$1"
fi

HOST_SRC=$(awk '/\/workspace /{print $4; exit}' /proc/self/mountinfo)
if [ -z "$HOST_SRC" ]; then
  HOST_SRC=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
fi

docker run --rm \
  -u 1000 \
  --device /dev/fuse \
  --cap-add SYS_ADMIN \
  --mount type=bind,src="$HOST_SRC",dst=/build \
  --mount type=bind,src=/usr/bin/fusermount3,dst=/usr/bin/fusermount3,readonly \
  --mount type=bind,src=/etc/passwd,dst=/etc/passwd,readonly \
  --security-opt label=disable \
  clang-tidy-image -c "
    /build/SimpleNetworkProtocol/build/SimpleNetworkProtocolTest $FILTER_ARG
  "
