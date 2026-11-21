# SentinelML

High-performance network intrusion detection and benchmarking system.

Research goal: maximise detection quality while minimising inference latency,
memory and compute, on a single Apple Silicon machine with no GPU/CUDA.

## Reference machine

All results in this repository come from one machine unless stated otherwise:
Apple M4 (4 performance + 6 efficiency cores), 16 GB unified memory, macOS 26.6,
Apple clang 21, Python 3.13. Run `python -m sentinelml.env` for the full report.

## Status

| Phase | Component | State |
|---|---|---|
| 1 | Project setup (Python env, C++/CMake toolchain, tests) | done — 7 pytest + 28 GoogleTest passing (release & ASan) |
| 2 | Data processing engine (CICIDS2017, UNSW-NB15) | in progress — dataset research and methodology drafted ([research/](research/)); no data downloaded yet |
| 3 | Detection engine (LR, RF, XGBoost, MLP, Isolation Forest) | not started |
| 4 | C++ inference engine (batching, threads, layout, NEON) | not started |
| 5 | Benchmarking framework | not started |
| 6 | Streaming replay prototype | not started |

## Layout

```
src/sentinelml/      Python package (data, models, benchmarks — added per phase)
tests/python/        pytest suites
cpp/include/sentinel Public C++ headers for the inference engine
cpp/src/             C++ implementation
cpp/tests/           GoogleTest suites
cpp/tools/           Small C++ executables (sentinel_info, later benchmark drivers)
configs/             Experiment configurations
data/raw/            Downloaded datasets (git-ignored)
data/processed/      Cleaned Parquet caches (git-ignored)
results/             Benchmark output (git-ignored; curated summaries go in docs/)
docs/                Engineering decisions and write-ups
research/            Dataset analysis and experimental methodology (with sources)
scripts/             Setup and verification scripts
```

## Setup (macOS, Apple Silicon)

```bash
xcode-select --install                 # if Command Line Tools are missing
brew install cmake ninja libomp        # libomp: OpenMP runtime used by XGBoost
scripts/setup_env.sh /Library/Frameworks/Python.framework/Versions/3.13/bin/python3.13
scripts/verify_env.sh
```

## C++ builds

```bash
cmake --preset release && cmake --build --preset release && ctest --preset release
cmake --preset asan    && cmake --build --preset asan    && ctest --preset asan
```

Only the `release` preset is ever used for performance numbers.

See [docs/decisions.md](docs/decisions.md) for the reasoning behind these choices and
[CONTRIBUTING.md](CONTRIBUTING.md) for commit conventions.
