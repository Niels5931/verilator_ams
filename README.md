# AMS Co-Simulation with Verilator + ngspice

> **⚠️ Vibe-coded disclaimer:** This project was built interactively with AI assistance ("vibe coding"). It is functional for demonstration purposes, but expect rough edges, incomplete features, and spots that need engineering review before production use. Treat it as a starting point, not a finished product.

This repo is a work-in-progress C++17 CMake project that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice) in a single process with lockstep synchronization per clock cycle.

Two driver patterns are supported:

- **C++-driven (legacy):** a per-example `main.cpp` loads the YAML config, registers Verilog port accessors/writers on an `AMSTestbench<Vtop>`, and runs the clock loop with `tb.step()` / `tb.run(n)`.
- **SystemVerilog/UVM-driven (DPI-C):** no C++ `main.cpp`. SystemVerilog imports `ams_dpi_pkg::*` and drives ngspice directly through `ams_init` / `ams_set_voltage` / `ams_run_analog` / `ams_get_voltage` / `ams_finish`. The C++ side is implemented by `src/ams_dpi.cpp` + `include/ams/ams_bridge.h` (`AMSBridge`, a DUT-less `IAMSTestbench`).

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

Requires CMake >= 3.19, C++17, Verilator, and the ngspice shared dev library (`libngspice`/`ngspice-0` + `sharedspice.h`). CMake fetches `yaml-cpp` automatically on first configure.

```bash
cmake -S . -B build
cmake --build build
```

### Python build driver (experimental)

`tools/ams_sim.py` is an experimental driver that builds and runs examples without CMake. It reads a per-example `bench.toml`, builds the AMS core library once into `build/core/libams_lib.a`, and uses Verilator's `--binary` mode to verilate, compile, and link each simulator. The driver auto-relinks the executable when the core library is newer, so edits to `src/*.cpp` take effect once `--force-core` is used to rebuild it.

```bash
./tools/ams_sim.py build <example>           # build once
./tools/ams_sim.py build <example> --force-core   # rebuild core lib + relink
./tools/ams_sim.py run   <example>           # build if needed, then run
```

For UVM examples, `UVM_ROOT` must point to your uvm-verilator source tree (the directory containing `uvm_pkg.sv`).

## Run an example

Each example has a helper script. Run it from the example directory because `spice_netlist` paths in `config.yaml` are relative to it.

CMake path:

```bash
./examples/r_ladder_dac/run.sh
./examples/adc_sar/run.sh
./examples/r_ladder_dac/run.sh --clean --verbose
```

Or manually:

```bash
cmake --build build --target ams_sim_r_ladder_dac
cd examples/r_ladder_dac
../../build/examples/r_ladder_dac/ams_sim_r_ladder_dac config.yaml
```

Python / UVM path (needs `UVM_ROOT`):

```bash
./examples/uvm_r_ladder_dac/run.sh --clean
# or:
UVM_ROOT=/path/to/uvm-verilator/src ./tools/ams_sim.py build uvm_r_ladder_dac
UVM_ROOT=/path/to/uvm-verilator/src ./tools/ams_sim.py run   uvm_r_ladder_dac
```

Sky130-based examples (`r_ladder_dac_sky130`, `two_stage_opamp`) require the `PDK_ROOT` environment variable to point at a Sky130 PDK install.

## Examples

C++-driven (legacy `main.cpp`):

- **`adc_sar`** -- Self-contained ideal 4-bit SAR ADC with behavioral DAC + comparator in ngspice; a good sanity check for a fresh environment.
- **`adc_sar_ladder`** -- SAR ADC that reuses the `r_ladder_dac` SPICE netlist via `.include` expansion in C++.
- **`r_ladder_dac`** -- Ideal 4-bit R-2R ladder DAC (no PDK required).
- **`r_ladder_dac_sky130`** -- Sky130 resistor-based R-2R ladder DAC (needs `PDK_ROOT`).
- **`two_stage_opamp`** -- Sky130 differential pair driven by a digital square wave (needs `PDK_ROOT`).

SystemVerilog/UVM-driven (DPI-C, no `main.cpp`):

- **`uvm_r_ladder_dac`** -- UVM-driven version of `r_ladder_dac` using a full agent: sequence, driver, monitor, scoreboard, and coverage subscriber. The driver pushes 4-bit DAC codes into ngspice, advances the analog sim one clock period per code, and checks `vout` against `vdd * code / 16`. All 16 codes PASS.

## General expectations

- The SPICE netlist must contain a `.tran` line with a total run time.
- ngspice is stepped by exactly one Verilator clock period per `step()` / `ams_run_analog()` call using `bg_run` / `bg_resume` breakpoints.
- The `.tran` end time only needs to be longer than the total simulated time; the co-simulation halts at each clock boundary.
- Digital inputs to ngspice are held stable for the full clock period.
- Analog outputs are sampled at the clock boundary.

## Wiring a new co-simulation

There are two patterns. See `AGENTS.md` for the full step-by-step instructions and gotchas; the short sketch below is enough to orient you.

### C++-driven (legacy)

Look at `examples/r_ladder_dac/main.cpp`:

1. `ams::loadConfig(argv[1])`.
2. `ams::AMSTestbench<Vtop>(config)` where `Vtop` is the Verilator top class.
3. Register `tb.output_accessors_[]` and `tb.input_writers_[]` **before** `init()`.
4. `tb.init(argc, argv)`, then `tb.openTrace("...vcd")`, then the loop with `tb.step()` / `tb.run(n)`.

### SystemVerilog/UVM-driven (DPI-C)

Look at `examples/uvm_r_ladder_dac/`:

1. `import ams_dpi_pkg::*;`.
2. Call `ams_init("config.yaml")` once in the test's `start_of_simulation_phase()` (not `run_phase`) so the AMS bridge is ready before the driver's `run_phase` samples `ams_get_vdd()`.
3. A UVM sequence sends transactions to the driver; the driver calls `ams_set_voltage()` for each ngspice EXTERNAL source, then `ams_run_analog(clock_period_ns)`, then `ams_get_voltage("node")`.
4. Monitor/scoreboard/coverage receive the result transaction over TLM analysis.
5. `ams_finish()` at end of simulation.

For build semantics, config YAML keys, SPICE/netlist conventions, and environment gotchas, see **`AGENTS.md`**.