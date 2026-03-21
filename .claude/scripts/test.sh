#!/usr/bin/env bash
# Run SimpleNetworkProtocolTest.
# Usage: test.sh [gtest_filter]
set -euo pipefail

if [ -n "${1:-}" ]; then
  SimpleNetworkProtocol/build/SimpleNetworkProtocolTest --gtest_filter="$1"
else
  SimpleNetworkProtocol/build/SimpleNetworkProtocolTest
fi
