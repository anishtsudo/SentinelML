#!/usr/bin/env bash
# Verify the full Phase 1 toolchain: Python env + tests, C++ build + tests.
set -euo pipefail

cd "$(dirname "$0")/.."

section() { printf '\n==== %s ====\n' "$1"; }

section "Toolchain"
uname -m
clang++ --version | head -1
cmake --version | head -1
echo "ninja $(ninja --version)"

section "Python environment report"
# shellcheck disable=SC1091
source .venv/bin/activate
python -m sentinelml.env

section "Python tests"
python -m pytest -v

section "C++ release build"
cmake --preset release
cmake --build --preset release

section "C++ build info"
./build/release/cpp/sentinel_info

section "C++ tests"
ctest --preset release

section "All Phase 1 checks passed"
