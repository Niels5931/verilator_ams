#!/usr/bin/env bash
set -e

# Run from the example directory:
#   UVM_ROOT=/path/to/uvm-verilator/src ./run.sh
#
# The UVM-driven R-2R ladder DAC example is built by the experimental Python
# driver (tools/ams_sim.py) instead of CMake.  This script just wraps it with
# the same --clean/--verbose interface as the other examples.

EXAMPLE_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${EXAMPLE_DIR}/../../build/uvm_r_ladder_dac"
CLEAN=0
VERBOSE=0

for arg in "$@"; do
    case "$arg" in
        --clean)      CLEAN=1 ;;
        --verbose|-v) VERBOSE=1 ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

if [ -z "${UVM_ROOT:-}" ]; then
    echo "Error: UVM_ROOT is not set." >&2
    echo "  Set it to the SystemVerilog UVM source tree used by Verilator." >&2
    exit 1
fi

for prog in verilator ngspice python3; do
    if ! command -v "$prog" >/dev/null 2>&1; then
        echo "Error: required program not found in PATH: $prog" >&2
        exit 1
    fi
done

if [ "$CLEAN" -eq 1 ]; then
    echo "=== Removing example build directory ==="
    rm -rf "$BUILD_DIR"
fi

if [ "$VERBOSE" -eq 1 ]; then
    echo "=== Note: --verbose is not yet wired to the Python driver; continuing ==="
fi

DRIVER="${EXAMPLE_DIR}/../../tools/ams_sim.py"

echo "=== Building UVM-driven R-ladder DAC AMS co-simulation ==="
python3 "$DRIVER" build uvm_r_ladder_dac

echo "=== Running UVM-driven R-ladder DAC AMS co-simulation ==="
cd "$EXAMPLE_DIR"
exec python3 "$DRIVER" run uvm_r_ladder_dac
