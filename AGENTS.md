# Agent Notes for AMS

> This repo is a **work in progress**: a C++17 library that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice).

## What this repo is

- Core AMS library: `include/ams/` + `src/ngspice_interface.cpp` + `src/ams_dpi.cpp`. Built as `libams_core.so` either by `tools/ams_sim.py build-core` or via the minimal top-level `CMakeLists.txt`.
- Example co-simulation under `examples/`:
  - `uvm_r_ladder_dac` — R-2R ladder DAC driven through the DPI-C AMS bridge. Split into `sv/` (SystemVerilog UVM) and `py/` (pyuvm/cocotb) flows that share the DUT Verilog and the SPICE netlist (`examples/uvm_r_ladder_dac/spice/r_ladder_dac.spice`). Each flow has its own `Makefile`; both auto-build `libams_core.so` via `tools/ams_sim.py build-core`.
- `ttsky-analog-template/` is a standalone Tiny Tapeout analog-project template (its own `.git`, `info.yaml`, GitHub Actions). It is **not** wired into the AMS build.

## Build

`tools/ams_sim.py` is now scoped to a single job: building `build/core/libams_core.so`. The `build` / `run <example>` subcommands and the `bench.toml` manifest have been retired; each example's Verilator (SV) or cocotb (pyuvm) build and run is owned by a `Makefile` in the example directory.

### `libams_core.so` (driver)

```bash
python3 tools/ams_sim.py build-core
```

Compiles `src/ngspice_interface.cpp` + `src/ams_dpi.cpp` and links the system ngspice + yaml-cpp. Discovery: `NGSPICE_HOME` / `YAML_CPP_HOME` first, then `pkg-config`, then standard system paths. No CMake, no `FetchContent`.

### CMake (alternative for `libams_core.so` only)

```bash
cmake -S . -B build
cmake --build build --target ams_core
```

- Requires CMake ≥ 3.19, C++17, and the ngspice shared dev library (`libngspice`/`ngspice-0` + `sharedspice.h`).
- Requires **system-installed** `yaml-cpp` (matched via `find_package(yaml-cpp)`); the driver does not vendor yaml-cpp via `FetchContent`.
- CMake searches for ngspice in `/usr/lib`, `/usr/local/lib`, `/usr/lib/x86_64-linux-gnu`, and `$NGSPICE_HOME/lib`.
- There are no per-example CMake targets.

## Run an example

The only supported example is `uvm_r_ladder_dac`, with two flows. Each flow is driven by a `Makefile` in its example directory. Run from that directory (`sv/` or `py/`) because `spice_netlist` paths in `config.yaml` are relative to it.

### SystemVerilog UVM flow

The `sv/Makefile` links ngspice + yaml-cpp strictly through env vars (no `pkg-config` fallback on that path). Required:

- `UVM_ROOT` — uvm-verilator source tree (contains `uvm_pkg.sv`).
- `NGSPICE_HOME` — ngspice install root (has `include/` and `lib/`).
- `YAML_CPP_HOME` — yaml-cpp install root (has `include/` and `lib/`).
- `VERILATOR_ROOT` — auto-detected via `verilator --getenv VERILATOR_ROOT`; override only if needed.

```bash
cd examples/uvm_r_ladder_dac/sv
UVM_ROOT=… NGSPICE_HOME=… YAML_CPP_HOME=… make
```

The Makefile depends on `libams_core.so` via an order-only rule that shells out to `tools/ams_sim.py build-core`, so you do not need to build it separately. `make clean` wipes `sim_build/` and `libams_core.so`.

### pyuvm / cocotb flow

```bash
cd examples/uvm_r_ladder_dac/py
make SIM=verilator
```

The `py/Makefile` auto-builds `libams_core.so` the same way. It needs `cocotb` and `pyuvm` importable by Python.

Requires `ngspice`, `yaml-cpp`, Verilator, and `UVM_ROOT` (for the SV flow) or `cocotb` + `pyuvm` (for the Python flow) to be available.

## Wiring a new co-simulation

There are two supported patterns, both DPI-C/driven and DUT-less (no per-example `main.cpp`):

### SystemVerilog-driven (UVM / DPI-C)

Look at `examples/uvm_r_ladder_dac/sv/`:

1. Import the DPI package: `import ams_dpi_pkg::*;`.
2. Call `ams_init(config_path)` once in the test `start_of_simulation_phase()` to load the YAML config and start ngspice. Calling it in `run_phase` races the driver, which samples `ams_get_vdd()` at the start of its own `run_phase` and would see `0`.
3. A UVM sequence (`r_ladder_dac_ramp_sequence`) sends 4-bit DAC codes to the agent's sequencer.
4. The UVM driver drives `vif.code`, pushes the code into ngspice with `ams_set_voltage`, advances the analog simulation with `ams_run_analog()` (one clock period per call), and captures `ams_get_voltage("vout")`.
5. The monitor receives the driver's result transaction via a TLM analysis port, checks the output against `vdd*code/16`, and forwards it to the coverage collector.
6. Call `ams_finish()` at the end of simulation.

The C++ side is implemented by `src/ams_dpi.cpp` + `include/ams/ams_bridge.h` (`AMSBridge`, a DUT-less implementation of `IAMSTestbench`). No per-example `main.cpp` is required. The example directory must also contain a `Makefile` that drives `verilator --binary` and depends on `libams_core.so` via `tools/ams_sim.py build-core`; see `examples/uvm_r_ladder_dac/sv/Makefile`.

### Python-driven (pyuvm / cocotb)

Look at `examples/uvm_r_ladder_dac/py/`:

1. Import the reusable ctypes binding: `from ams import get_bridge`. `tools/ams_py/ams/` is the Python counterpart to `include/ams/ams_dpi_pkg.sv`.
2. Call `get_bridge().init(config_path)` once in the test `start_of_simulation_phase()` to load the YAML config and start ngspice.
3. A pyuvm sequence (`RLDacRampSequence`) sends 4-bit DAC codes to the agent's sequencer.
4. The pyuvm driver drives the DUT's `code` port via cocotb, pushes the bits into ngspice with `ams_set_voltage`, advances the analog simulation with `ams_run_analog()`, and captures `ams_get_voltage("vout")`.
5. The monitor receives the sampled result directly from the driver via a TLM analysis port and forwards it to the scoreboard and coverage collector.
6. Call `get_bridge().finish()` at the end of simulation.

Both the SV and Python flows link against the same `build/core/libams_core.so` (Verilator links it into the SV simulator binary; Python loads it via `ctypes`). The DUT Verilog (`r_ladder_dac_dut.sv`) and the SPICE netlist (`spice/r_ladder_dac.spice`) are shared between the two flows.

## Config YAML semantics

Under the `ams_cosim` key:

- `clock_period_ns`, `vdd`, `spice_netlist` are required.
- `digital_to_analog` entries map a Verilog output to an ngspice `EXTERNAL` voltage source.
- `analog_to_digital` entries map an ngspice node voltage to a Verilog input.
- `digital_to_analog` and `analog_to_digital` are optional; pure DPI-C/SV-driven co-simulations can drive and sample ngspice directly without using the C++ port accessor map.
- `width` controls scaling:
  - `1` — logic level: `0` or `vdd`.
  - `32` — signed fixed-point Q16.16 via `to_fixed()`/`from_fixed()`.
  - `1 < width < 32` — unsigned ratio to `vdd`.

## SPICE/netlist conventions

- Digital Verilog outputs drive ngspice through voltage sources declared `EXTERNAL`, e.g. `Vdigital_in Vp 0 EXTERNAL`. The `spice_source` name must match the source name.
- Analog node names are matched case-insensitively; a leading `v` on external source names is stripped during normalization.
- Every netlist needs a `.tran` line; ngspice is advanced one clock period per `step()` using `bg_run` / `bg_resume` breakpoints.
- `ngSpice_Circ` does **not** pre-process `.include` directives. Expand them before passing the netlist vector.
  When doing so, put wrapper lines **before** an included file that ends with `.end`; anything after `.end` is ignored.

## Environment gotchas

- The ngspice shared library is initialised once per process. `NgSpiceInterface` routes callbacks through the active instance so loops that create a fresh `AMSBridge` per voltage/netlist do not call back into a destroyed object.
- There is no test runner, linter, or formatter configured. The `uvm_r_ladder_dac` examples are the only executable verification.