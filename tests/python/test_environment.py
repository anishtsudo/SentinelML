"""Phase 1 environment checks.

These are deliberately functional (train + predict), not just imports: an
import can succeed while the native runtime underneath (e.g. libomp for
XGBoost) fails on first use.
"""

import platform
import sys

import numpy as np
import pytest

from sentinelml import env


def test_python_version_supported():
    assert (3, 12) <= sys.version_info[:2] < (3, 14)


def test_native_arm64_not_rosetta():
    assert platform.machine() == "arm64", "Python is not a native arm64 build"
    assert not env.running_under_rosetta(), "Python is running under Rosetta 2"


def test_all_libraries_importable():
    missing = [name for name, ver in env.library_versions().items() if ver is None]
    assert not missing, f"missing libraries: {missing}"


def test_numpy_uses_accelerate():
    blas = env.numpy_blas()
    if blas is None:
        pytest.skip("could not read NumPy build config")
    # Not a hard requirement for correctness, but OpenBLAS here would change
    # every linear-algebra-heavy baseline we measure later.
    assert "accelerate" in blas.lower(), f"NumPy BLAS is {blas!r}, expected Accelerate"


@pytest.fixture
def toy_data():
    rng = np.random.default_rng(0)
    X = rng.normal(size=(500, 8)).astype(np.float32)
    y = (X[:, 0] + 0.5 * X[:, 1] > 0).astype(np.int32)
    return X, y


def test_sklearn_fit_predict(toy_data):
    from sklearn.ensemble import RandomForestClassifier

    X, y = toy_data
    clf = RandomForestClassifier(n_estimators=10, random_state=0, n_jobs=2).fit(X, y)
    assert clf.score(X, y) > 0.9


def test_xgboost_fit_predict_multithreaded(toy_data):
    # n_jobs>1 exercises the OpenMP runtime, which is the usual failure point on macOS.
    import xgboost as xgb

    X, y = toy_data
    clf = xgb.XGBClassifier(n_estimators=20, max_depth=3, n_jobs=4, tree_method="hist")
    clf.fit(X, y)
    assert clf.score(X, y) > 0.9


def test_environment_report_is_complete():
    report = env.environment_report()
    for key in ("python", "machine", "numpy_blas", "cpu", "libraries", "total_memory_bytes"):
        assert key in report
    assert report["cpu"]["logical_cpus"] >= 1
