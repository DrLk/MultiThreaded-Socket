#!/usr/bin/env bash
# Run SimpleNetworkProtocolTest inside Docker.
# Usage: test.sh [gtest_filter]
set -euo pipefail

FILTER_ARG=""
if [ -n "${1:-}" ]; then
  FILTER_ARG="--gtest_filter=$1"
fi

docker run --rm \
  -u 1000 \
  --device /dev/fuse \
  --cap-add SYS_ADMIN \
  --mount type=bind,src="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)",dst=/build \
  --mount type=bind,src=/usr/bin/fusermount3,dst=/usr/bin/fusermount3,readonly \
  clang-tidy-image -c "
    /build/SimpleNetworkProtocol/build/SimpleNetworkProtocolTest $FILTER_ARG
  "
