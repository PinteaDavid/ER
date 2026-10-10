#!/usr/bin/env bash
# Compileaza si porneste aplicatia (macOS / Linux)
set -e
cd "$(dirname "$0")"
if ! command -v cmake >/dev/null 2>&1; then
  echo "CMake nu este instalat. Ruleaza: brew install cmake"
  exit 1
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/descompunere
