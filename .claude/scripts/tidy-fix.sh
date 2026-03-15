#!/usr/bin/env bash
# Run clang-tidy --fix + clang-format inside Docker.
# Usage: tidy-fix.sh <HOST_SRC> <file1> [file2 ...]
set -euo pipefail

HOST_SRC="$1"; shift
FILES="$*"

# Prefix each file with /build/ for use inside the container
CONTAINER_FILES=$(echo "$FILES" | tr ' ' '\n' | sed 's|^|/build/|')

docker run --rm \
  -u 1000 \
  --mount type=bind,src="$HOST_SRC",dst=/build \
  clang-tidy-image -c "
    cd /build/SimpleNetworkProtocol/build &&
    CC=clang CXX=clang++ cmake -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_PRECOMPILE_HEADERS=OFF .. &&
    cd /build &&
    echo '$CONTAINER_FILES' | xargs -P\$(nproc) -I{} clang-tidy -p SimpleNetworkProtocol/build --fix --fix-errors --use-color {} ; true &&
    echo '$CONTAINER_FILES' | xargs -P\$(nproc) clang-format -i
  "
