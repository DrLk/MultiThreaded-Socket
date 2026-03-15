#!/usr/bin/env bash
# Resolves host-side mount path and lists changed C/C++ files vs master.
# Output: first line is HOST_SRC=<path>, then one file per line.
set -euo pipefail

HOST_SRC=$(awk '/\/workspace /{print $4; exit}' /proc/self/mountinfo)
echo "HOST_SRC=$HOST_SRC"

{ git diff master HEAD --name-only -- "*.cpp" "*.cc" "*.cxx" "*.c" "*.hpp" "*.hxx" "*.h"
  git diff --name-only -- "*.cpp" "*.cc" "*.cxx" "*.c" "*.hpp" "*.hxx" "*.h"
} | sort -u
