# Engineering decision log

Short records of non-obvious choices, so later results can be interpreted in context.

## D1 — Python 3.13 in a plain `venv`, not conda base or Homebrew 3.14
The machine has several interpreters (conda base 3.14, Homebrew 3.14, python.org 3.13,
system 3.9). Conda base was first on `PATH`, so `python3` was ambiguous. We pin an
explicit interpreter and isolate with `venv`. 3.13 rather than 3.14 because the
scientific stack (XGBoost, scikit-learn, pyarrow) gets the most testing on the
version before the newest. `setup_env.sh` refuses non-arm64 interpreters, since an
x86 Python under Rosetta would run correctly but invalidate every timing.

## D2 — Lower-bound deps in `pyproject.toml`, exact pins in `requirements.lock.txt`
The package declares what it needs; the lock file records what was actually measured.
Benchmark output will also embed `sentinelml.env.environment_report()`.

## D3 — C++20, CMake presets, Ninja, GoogleTest via FetchContent
Presets make "the release build" a single named thing instead of a set of flags
someone has to remember. GoogleTest is pinned to a tagged release and built from
source with the same compiler, avoiding ABI mismatch with a Homebrew copy.

## D4 — `-mcpu=native` on by default, off for sanitizer builds
Lets the compiler use every instruction the M4 supports. The trade-off is that
binaries are not portable to older chips, which is acceptable for a local research
benchmark but must be stated when reporting numbers.

## D5 — No OpenMP in our own C++ code (for now)
Apple clang ships without OpenMP. We will use `std::thread`/`std::jthread` and a
small thread pool, which keeps the build dependency-free and lets us control
scheduling explicitly. XGBoost's wheel still needs Homebrew `libomp` at runtime.

## D6 — Benchmarks on macOS have specific hazards
- No thread-affinity API: threads beyond the P-core count spill onto E-cores. The
  reference machine has only **4 P-cores** (base M4, verified via `sysctl`), so
  thread-scaling curves are expected to bend after 4 threads. Measurements beyond
  that point mix P- and E-core behaviour and must be labelled as such.
- 16 GB unified memory: dataset loading must be dtype-aware (float32, chunked CSV
  parsing) rather than loading every CSV with pandas defaults (float64/object).
- Thermal/power state matters: run on AC power, Low Power Mode off, warm-up runs.
- Debug builds and sanitizer builds are never benchmarked.

## D7 — `enable_testing()` at top level; GoogleTest as a SYSTEM dependency
CTest only discovers tests through the top-level directory, so `enable_testing()` lives
in the root `CMakeLists.txt` (calling it only in `cpp/` produced "No tests were found").
GoogleTest is declared `SYSTEM` so our strict warning flags don't fire on its headers,
and clang 21's new `-Wcharacter-conversion` is silenced on the `gtest` target alone
(it fires inside GoogleTest 1.15.2's own sources). Result: release builds are clean
under `-DSENTINEL_WARNINGS_AS_ERRORS=ON`.
