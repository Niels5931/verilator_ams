#!/usr/bin/env bash
set -e

# Run from the example directory:
#   ./run.sh [--clean]

EXAMPLE_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${EXAMPLE_DIR}/../../.." && pwd)"
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        --clean)      CLEAN=1 ;;
        --verbose|-v) ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

for prog in verilator ngspice python3 make; do
    if ! command -v "$prog" >/dev/null 2>&1; then
        echo "Error: required program not found in PATH: $prog" >&2
        exit 1
    fi
done

python3 -c "import cocotb" 2>/dev/null || { echo "Error: python3 -m pip install cocotb" >&2; exit 1; }
python3 -c "import pyuvm"   2>/dev/null || { echo "Error: python3 -m pip install pyuvm"   >&2; exit 1; }

if [ "$CLEAN" -eq 1 ]; then
    echo "=== Removing build artifacts ==="
    rm -rf "${REPO_ROOT}/build/core/libams_core.so"
    rm -rf "${EXAMPLE_DIR}/sim_build"
    rm -f  "${EXAMPLE_DIR}/results.xml"
fi

export LD_LIBRARY_PATH="${REPO_ROOT}/build/core${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PYTHONPATH="${REPO_ROOT}/tools/ams_py:${EXAMPLE_DIR}${PYTHONPATH:+:$PYTHONPATH}"

# Let the Makefile locate cocotb's makefiles without requiring cocotb-config
# to be on the shell PATH.
COCOTB_MAKEFILES=$(python3 -m cocotb_tools.config --makefiles) || {
    echo "Error: could not locate cocotb makefiles. Is cocotb installed?" >&2
    exit 1
}
export COCOTB_MAKEFILES

echo "=== Building libams_core.so ==="
python3 "${REPO_ROOT}/tools/ams_sim.py" build-core

echo "=== Building and running pyuvm R-ladder DAC AMS co-simulation ==="
cd "$EXAMPLE_DIR"
exec make SIM=verilator
