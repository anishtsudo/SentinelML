#!/usr/bin/env bash
# Create the Python virtual environment and install SentinelML in editable mode.
# Usage: scripts/setup_env.sh [path-to-python]   (default: python3.13)
set -euo pipefail

cd "$(dirname "$0")/.."

PYTHON="${1:-python3.13}"

if ! command -v "$PYTHON" >/dev/null 2>&1; then
    echo "error: '$PYTHON' not found. Pass an interpreter path, e.g. scripts/setup_env.sh /path/to/python3.13" >&2
    exit 1
fi

ARCH="$("$PYTHON" -c 'import platform; print(platform.machine())')"
if [[ "$ARCH" != "arm64" ]]; then
    echo "error: $PYTHON reports machine '$ARCH'. Use a native arm64 Python (not x86/Rosetta)." >&2
    exit 1
fi

echo ">> Using $("$PYTHON" -c 'import sys; print(sys.executable, sys.version.split()[0])')"

if [[ ! -d .venv ]]; then
    "$PYTHON" -m venv .venv
fi

# shellcheck disable=SC1091
source .venv/bin/activate

python -m pip install --upgrade pip
python -m pip install -e ".[dev]"

# Exact versions used, for reproducibility. Commit this file.
python -m pip freeze --exclude-editable > requirements.lock.txt

echo ">> Done. Activate with: source .venv/bin/activate"
