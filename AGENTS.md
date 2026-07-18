# Agent Notes for AMS

> This repo is a **work in progress**: a C++17 library that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice).

## What this repo is

- Core AMS library: `include/ams/` + `src/ngspice_interface.cpp` + `src/ams_dpi.cpp`. Built as `libams_core.so` either by `tools/ams_sim.py build-core` or via the minimal top-level `CMakeLists.txt`.
- Example co-simulation under `examples/`:
  - `uvm_r_ladder_dac` — R-2R ladder DAC driven through the DPI-C AMS bridge (`tools/ams_sim.py`). The example is split into `sv/` (SystemVerilog UVM) and `py/` (pyuvm/cocotb) flows that share the DUT Verilog and the SPICE netlist (`examples/uvm_r_ladder_dac/spice/r_ladder_dac.spice`).
- `ttsky-analog-template/` is a standalone Tiny Tapeout analog-project template (its own `.git`, `info.yaml`, GitHub Actions). It is **not** wired into the AMS build.

## Build

The primary build path is the Python driver (`tools/ams_sim.py`). CMake is retained only as an alternative way to produce `libams_core.so`; the supported example (`uvm_r_ladder_dac`) is **not** built by CMake — it is built by the driver via Verilator `--binary` (SV) or the cocotb Makefile (pyuvm).

### Python driver (primary)

```bash
python3 tools/ams_sim.py build-core
```

Builds `build/core/libams_core.so` from `src/ngspice_interface.cpp` + `src/ams_dpi.cpp`, linking the system ngspice + yaml-cpp. No CMake, no `FetchContent`.

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

The only supported example is `uvm_r_ladder_dac`, with two flows.

For the SV UVM flow, use the `sv/run.sh` script. `UVM_ROOT` must be defined and point at your uvm-verilator source tree:

```bash
./examples/uvm_r_ladder_dac/sv/run.sh --clean
```

Or invoke the driver directly:

```bash
UVM_ROOT=$UVM_ROOT ./tools/ams_sim.py build uvm_r_ladder_dac/sv
UVM_ROOT=$UVM_ROOT ./tools/ams_sim.py run   uvm_r_ladder_dac/sv
```

For the pyuvm flow, the driver only builds `libams_core.so`; the cocotb Makefile in the example directory builds the Verilated DUT module and runs the Python testbench:

```bash
python3 tools/ams_sim.py build-core
./examples/uvm_r_ladder_dac/py/run.sh
```

The `bench.toml` manifest has a `driver` field: `uvm` for the SystemVerilog UVM flow, or `pyuvm` for the pyuvm/cocotb flow.

Requires `ngspice`, `yaml-cpp`, Verilator, and UVM (for the SV flow) or `cocotb` + `pyuvm` (for the Python flow) to be discoverable (system packages or `NGSPICE_HOME` / `YAML_CPP_HOME`).

Run from the example directory (`sv/` or `py/`) because `spice_netlist` paths in `config.yaml` are relative to that directory.

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

The C++ side is implemented by `src/ams_dpi.cpp` + `include/ams/ams_bridge.h` (`AMSBridge`, a DUT-less implementation of `IAMSTestbench`). No per-example `main.cpp` is required.

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