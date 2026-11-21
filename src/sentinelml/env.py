"""Environment introspection.

Every benchmark result we produce later will embed this report, so a number
can always be traced back to the exact interpreter, library versions, BLAS
backend and CPU topology that produced it.

Run directly:  python -m sentinelml.env
"""

from __future__ import annotations

import importlib
import json
import os
import platform
import subprocess
import sys
from typing import Any

_LIBRARIES = ("numpy", "pandas", "pyarrow", "sklearn", "xgboost", "matplotlib", "psutil")


def _sysctl(key: str) -> str | None:
    """Read a macOS sysctl value; returns None on other platforms or failure."""
    if sys.platform != "darwin":
        return None
    try:
        out = subprocess.run(
            ["sysctl", "-n", key], capture_output=True, text=True, check=True, timeout=2
        )
    except (OSError, subprocess.SubprocessError):
        return None
    return out.stdout.strip() or None


def running_under_rosetta() -> bool:
    """True if this is an x86_64 process translated by Rosetta 2.

    An x86 Python on an M-series Mac runs silently but much slower and without
    NEON, which would invalidate every performance number we collect.
    """
    return _sysctl("sysctl.proc_translated") == "1"


def numpy_blas() -> str | None:
    """Name of the BLAS library NumPy was built against (e.g. 'accelerate')."""
    import numpy as np

    try:
        cfg = np.show_config(mode="dicts")
        return cfg["Build Dependencies"]["blas"]["name"]
    except Exception:  # noqa: BLE001 - config layout is not a stable API
        return None


def library_versions() -> dict[str, str | None]:
    versions: dict[str, str | None] = {}
    for name in _LIBRARIES:
        try:
            versions[name] = getattr(importlib.import_module(name), "__version__", "unknown")
        except ImportError:
            versions[name] = None
    return versions


def cpu_topology() -> dict[str, Any]:
    """Logical CPUs plus Apple Silicon performance/efficiency core split.

    P- vs E-cores matter for multithreading benchmarks: macOS gives no thread
    affinity API, so thread counts beyond the P-core count will land on slower
    E-cores and bend scaling curves.
    """
    return {
        "logical_cpus": os.cpu_count(),
        "brand": _sysctl("machdep.cpu.brand_string"),
        "performance_cores": _sysctl("hw.perflevel0.physicalcpu"),
        "efficiency_cores": _sysctl("hw.perflevel1.physicalcpu"),
        "l1d_bytes_pcore": _sysctl("hw.perflevel0.l1dcachesize"),
        "l2_bytes_pcore": _sysctl("hw.perflevel0.l2cachesize"),
        "cache_line_bytes": _sysctl("hw.cachelinesize"),
    }


def environment_report() -> dict[str, Any]:
    import psutil

    return {
        "python": sys.version.split()[0],
        "python_executable": sys.executable,
        "platform": platform.platform(),
        "machine": platform.machine(),
        "rosetta": running_under_rosetta(),
        "numpy_blas": numpy_blas(),
        "total_memory_bytes": psutil.virtual_memory().total,
        "cpu": cpu_topology(),
        "libraries": library_versions(),
    }


if __name__ == "__main__":
    print(json.dumps(environment_report(), indent=2))
