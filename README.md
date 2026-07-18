# AMS Co-Simulation with Verilator + ngspice

> **⚠️ Vibe-coded disclaimer:** This project was built interactively with AI assistance ("vibe coding"). It is functional for demonstration purposes, but expect rough edges, incomplete features, and spots that need engineering review before production use. Treat it as a starting point, not a finished product.

This repo is a work-in-progress C++17 library that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice) in a single process with lockstep synchronization per clock cycle.

Two DPI-C-driven patterns are supported (no per-example `main.cpp` required):

- **SystemVerilog/UVM-driven (DPI-C):** SystemVerilog imports `ams_dpi_pkg::*` and drives ngspice directly through `ams_init` / `ams_set_voltage` / `ams_run_analog` / `ams_get_voltage` / `ams_finish`. The C++ side is implemented by `src/ams_dpi.cpp` + `include/ams/ams_bridge.h` (`AMSBridge`, a DUT-less `IAMSTestbench`).
- **Python-driven (pyuvm / cocotb):** a pyuvm testbench loads the same `libams_core.so` through the ctypes binding in `tools/ams_py/ams/` and drives ngspice via the equivalent `init` / `set_voltage` / `run_analog` / `get_voltage` / `finish` methods.

## What it does

- Verilator runs the digital design in the main thread.
- ngspice runs the analog netlist in a background thread, controlled through the shared library API (`bg_run`, `bg_resume`, `bg_halt`).
- On every clock period:
  1. Verilog outputs are sampled and converted to analog voltages.
  2. ngspice advances by exactly one clock period.
  3. Analog node voltages are sampled and converted back to digital values.
  4. The Verilog design is clocked with the new inputs.

The boundary supports:

- 1-bit logic signals (`0` or `vdd`).
- 32-bit Q16.16 fixed-point analog values (`to_fixed()` / `from_fixed()`).
- Other widths are passed through as raw integers (design-specific conversion should live in the example harness).

## Build

`build/core/libams_core.so` is built by the Python driver:

```bash
python3 tools/ams_sim.py build-core
```

This compiles `src/ngspice_interface.cpp` + `src/ams_dpi.cpp` and links the system ngspice + yaml-cpp. `build-core` discovers ngspice and yaml-cpp via `NGSPICE_HOME` / `YAML_CPP_HOME` first, then `pkg-config`, then a few standard system paths (see `tools/ams_sim.py` for the exact list). Each example's Verilator/cocotb build is driven by its own `Makefile` in the example directory.

CMake is retained only as an alternative way to produce `libams_core.so`; the supported example (`uvm_r_ladder_dac`) is **not** built by CMake.

```bash
cmake -S . -B build
cmake --build build --target ams_core
```

- Requires CMake ≥ 3.19, C++17, and the ngspice shared dev library (`libngspice`/`ngspice-0` + `sharedspice.h`).
- Requires **system-installed** `yaml-cpp` (matched via `find_package(yaml-cpp)`); no `FetchContent`, no network access.
- There are no per-example CMake targets.

## Run an example

The only supported example is `uvm_r_ladder_dac`, with two flows. Each flow is driven by a `Makefile` in the example directory. `spice_netlist` paths in `config.yaml` are relative to that directory, so just `cd` into `sv/` or `py/` and run `make`.

### SystemVerilog UVM flow

The SV Makefile links ngspice and yaml-cpp strictly through env vars — there is no `pkg-config` fallback on that path. Required variables:

- `UVM_ROOT` — your uvm-verilator source tree (contains `uvm_pkg.sv`).
- `NGSPICE_HOME` — ngspice install root (has `include/` and `lib/`).
- `YAML_CPP_HOME` — yaml-cpp install root (has `include/` and `lib/`).
- `VERILATOR_ROOT` — auto-detected from `verilator --getenv VERILATOR_ROOT`; override only if needed.

```bash
cd examples/uvm_r_ladder_dac/sv
UVM_ROOT=/path/to/uvm-verilator/src \
NGSPICE_HOME=/path/to/ngspice \
YAML_CPP_HOME=/path/to/yaml-cpp \
make
```

The Makefile auto-builds `libams_core.so` via `tools/ams_sim.py build-core` (which still uses `pkg-config`/system-path discovery) if it is missing or stale, so you do not need to run `build-core` separately. Use `make clean` to wipe `sim_build/` and `libams_core.so`.

### pyuvm / cocotb flow

```bash
cd examples/uvm_r_ladder_dac/py
make SIM=verilator
```

The py Makefile auto-builds `libams_core.so` the same way the SV Makefile does. It needs `cocotb` and `pyuvm` importable by Python.

## Examples

- **`uvm_r_ladder_dac`** — R-2R ladder DAC driven through the DPI-C AMS bridge using a full agent: sequence, driver, monitor, scoreboard, and coverage subscriber. The driver pushes 4-bit DAC codes into ngspice, advances the analog sim one clock period per code, and checks `vout` against `vdd * code / 16`. All 16 codes PASS. Split into `sv/` (SystemVerilog UVM) and `py/` (pyuvm/cocotb) flows that share the DUT Verilog and the SPICE netlist (`spice/r_ladder_dac.spice`).
- `ttsky-analog-template/` is a standalone Tiny Tapeout analog-project template (its own `.git`, `info.yaml`, GitHub Actions), not wired into the AMS build.

## General expectations

- The SPICE netlist must contain a `.tran` line with a total run time.
- ngspice is stepped by exactly one Verilator clock period per `step()` / `ams_run_analog()` call using `bg_run` / `bg_resume` breakpoints.
- The `.tran` end time only needs to be longer than the total simulated time; the co-simulation halts at each clock boundary.
- Digital inputs to ngspice are held stable for the full clock period.
- Analog outputs are sampled at the clock boundary.

## Wiring a new co-simulation

There are two supported DPI-C-driven patterns. See `AGENTS.md` for the full step-by-step instructions and gotchas; the short sketch below is enough to orient you.

### SystemVerilog/UVM-driven (DPI-C)

Look at `examples/uvm_r_ladder_dac/sv/`:

1. `import ams_dpi_pkg::*;`.
2. Call `ams_init("config.yaml")` once in the test's `start_of_simulation_phase()` (not `run_phase`) so the AMS bridge is ready before the driver's `run_phase` samples `ams_get_vdd()`.
3. A UVM sequence sends transactions to the driver; the driver calls `ams_set_voltage()` for each ngspice EXTERNAL source, then `ams_run_analog(clock_period_ns)`, then `ams_get_voltage("node")`.
4. Monitor/scoreboard/coverage receive the result transaction over TLM analysis.
5. `ams_finish()` at end of simulation.

### Python-driven (pyuvm / cocotb)

Look at `examples/uvm_r_ladder_dac/py/`:

1. `from ams import get_bridge` (the ctypes binding in `tools/ams_py/ams/`).
2. Call `get_bridge().init("config.yaml")` once in the test's `start_of_simulation_phase()`.
3. A pyuvm sequence sends transactions to the driver; the driver drives the DUT's ports via cocotb, calls `set_voltage()` / `run_analog()` / `get_voltage("node")`.
4. Monitor/scoreboard/coverage receive the result transaction over TLM analysis.
5. `get_bridge().finish()` at end of simulation.

For build semantics, config YAML keys, SPICE/netlist conventions, and environment gotchas, see **`AGENTS.md`**.