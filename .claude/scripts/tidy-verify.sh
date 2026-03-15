#!/usr/bin/env bash
# Run clang-tidy (no --fix) to verify remaining diagnostics.
# Usage: tidy-verify.sh <HOST_SRC> <file1> [file2 ...]
set -euo pipefail

HOST_SRC="$1"; shift
FILES="$*"

# Prefix each file with /build/ for use inside the container
CONTAINER_FILES=$(echo "$FILES" | tr ' ' '\n' | sed 's|^|/build/|')

docker run --rm \
  -u 1000 \
  --mount type=bind,src="$HOST_SRC",dst=/build \
  clang-tidy-image -c "
    cd /build &&
    echo '$CONTAINER_FILES' | xargs -P\$(nproc) -I{} clang-tidy -p SimpleNetworkProtocol/build --use-color {}
  "
