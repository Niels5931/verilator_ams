#!/usr/bin/env bash
set -e

# Run from the example directory:
#   ./run.sh

EXAMPLE_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${EXAMPLE_DIR}/build"
CLEAN=0
VERBOSE=0

for arg in "$@"; do
    case "$arg" in
        --clean)   CLEAN=1 ;;
        --verbose|-v) VERBOSE=1 ;;
        *) echo "Unknown argument: $arg" >&2; exit 1 ;;
    esac
done

if [ "$CLEAN" -eq 1 ]; then
    echo "=== Removing build directory ==="
    rm -rf "$BUILD_DIR"
fi

MAKEFLAGS=""
if [ "$VERBOSE" -eq 1 ]; then
    MAKEFLAGS="VERBOSE=1"
fi

echo "=== Creating build directory ==="
mkdir -p "$BUILD_DIR"

echo "=== Running CMake configure ==="
cmake -S "$EXAMPLE_DIR" -B "$BUILD_DIR"

echo "=== Building project ==="
cmake --build "$BUILD_DIR" --target ams_sim_adc_sar -- $MAKEFLAGS

echo "=== Running SAR ADC AMS co-simulation ==="
cd "$EXAMPLE_DIR"
exec "$BUILD_DIR/ams_sim_adc_sar" "config.yaml"
