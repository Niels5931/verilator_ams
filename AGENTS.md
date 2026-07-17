# Agent Notes for AMS

> This repo is a **work in progress**: a C++17 CMake project that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice).

## What this repo is

- Core AMS library: `include/ams/` + `src/ngspice_interface.cpp`.
- Example co-simulations under `examples/`:
  - `adc_sar` — Ideal 4-bit SAR ADC loop.
  - `adc_sar_ladder` — SAR ADC that reuses the `r_ladder_dac` SPICE netlist.
  - `r_ladder_dac` — Ideal 4-bit R-2R ladder DAC (no PDK required).
  - `r_ladder_dac_sky130` — Sky130 resistor-based R-2R ladder DAC.
  - `two_stage_opamp` — Sky130 differential pair, digital square-wave drive.
  - `uvm_r_ladder_dac` — UVM-driven version of `r_ladder_dac` using the DPI-C AMS bridge (`tools/ams_sim.py`). No C++ `main.cpp`; a UVM agent (sequence, driver, monitor, and coverage subscriber) drives the 4-bit DAC codes and checks the analog output.
- `ttsky-analog-template/` is a standalone Tiny Tapeout analog-project template (its own `.git`, `info.yaml`, GitHub Actions). It is **not** wired into the CMake build.

## Build

```bash
cmake -S . -B build
cmake --build build
```

- Requires CMake ≥ 3.19, C++17, Verilator, and the ngspice shared dev library (`libngspice`/`ngspice-0` + `sharedspice.h`).
- CMake fetches `yaml-cpp` via `FetchContent` on first configure; network access required.
- `yaml-cpp-0.9.0` is pinned in `CMakeLists.txt`.
- CMake searches for ngspice in `/usr/lib`, `/usr/local/lib`, `/usr/lib/x86_64-linux-gnu`, and `$NGSPICE_HOME/lib`.

## Run an example

Each example has a helper script:

```bash
./examples/r_ladder_dac/run.sh
./examples/adc_sar/run.sh
./examples/adc_sar_ladder/run.sh
./examples/r_ladder_dac/run.sh --clean --verbose
```

To build/run a single example manually:

```bash
cmake --build build --target ams_sim_r_ladder_dac
cd examples/r_ladder_dac
../../build/examples/r_ladder_dac/ams_sim_r_ladder_dac config.yaml
```

The executable expects `config.yaml` as its only argument. Run it from the example directory because `spice_netlist` paths in `config.yaml` are relative to that directory.

### Python build driver (experimental)

`tools/ams_sim.py` is an experimental Python driver that builds and runs examples without CMake.  It reads a per-example `bench.toml` manifest, builds a shared AMS core library once, and uses Verilator's `--binary` mode to compile the simulator.

For UVM examples, use the per-example `run.sh` script. `UVM_ROOT` must be defined and point to your uvm-verilator source tree:

```bash
./examples/uvm_r_ladder_dac/run.sh --clean
```

Or invoke the driver directly:

```bash
UVM_ROOT=$UVM_ROOT ./tools/ams_sim.py build uvm_r_ladder_dac
UVM_ROOT=$UVM_ROOT ./tools/ams_sim.py run   uvm_r_ladder_dac
```

Requires `ngspice`, `yaml-cpp`, Verilator, and UVM to be discoverable (system packages or `NGSPICE_HOME` / `YAML_CPP_HOME`).

## Wiring a new co-simulation

There are two patterns:

### C++-driven (legacy examples)

Look at `examples/*/main.cpp`:

1. Load config: `ams::loadConfig(argv[1])`.
2. Instantiate `ams::AMSTestbench<Vtop>(config)` where `Vtop` is the Verilator top-class name (e.g. `Vams_top`).
3. Register port accessors/writers **before** `init()`:
   - `tb.output_accessors_["verilog_port"] = [](const Vtop* d) -> uint32_t { ... };` for `digital_to_analog` signals.
   - `tb.input_writers_["verilog_port"] = [](Vtop* d, uint32_t val) { ... };` for `analog_to_digital` signals.
4. `tb.init(argc, argv)` loads the SPICE netlist and resets the DUT for 5 cycles.
5. Call `tb.openTrace("ams_waveforms.vcd")` before stepping if you want a VCD trace (written to the current working directory).
6. Drive the loop with `tb.step()` or `tb.run(n_cycles)`.

`init()` also accepts an optional `std::vector<std::string>` netlist if you need to build/modify the SPICE text in C++ (see `adc_sar` and `adc_sar_ladder`).

### SystemVerilog-driven (UVM / DPI-C)

Look at `examples/uvm_r_ladder_dac/`:

1. Import the DPI package: `import ams_dpi_pkg::*;`.
2. Call `ams_init(config_path)` once in the test `start_of_simulation_phase()` to load the YAML config and start ngspice. Calling it in `run_phase` races the driver, which samples `ams_get_vdd()` at the start of its own `run_phase` and would see `0`.
3. A UVM sequence (`r_ladder_dac_ramp_sequence`) sends 4-bit DAC codes to the agent's sequencer.
4. The UVM driver drives `vif.code`, pushes the code into ngspice with `ams_set_voltage`, advances the analog simulation with `ams_run_analog()` (one clock period per call), and captures `ams_get_voltage("vout")`.
5. The monitor receives the driver's result transaction via a TLM analysis port, checks the output against `vdd*code/16`, and forwards it to the coverage collector.
6. Call `ams_finish()` at the end of simulation.

The C++ side is implemented by `src/ams_dpi.cpp` + `include/ams/ams_bridge.h` (`AMSBridge`, a DUT-less implementation of `IAMSTestbench`). No per-example `main.cpp` is required.

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
- `ngSpice_Circ` does **not** pre-process `.include` directives. Expand them in C++ before passing the netlist vector (see `adc_sar_ladder`).
  When doing so, put wrapper lines **before** an included file that ends with `.end`; anything after `.end` is ignored.

## Environment gotchas

- The `two_stage_opamp` netlist hard-codes the Sky130 PDK path:
  ```spice
  .lib {PDK_ROOT}/open_pdks/sky130A/libs.tech/combined/sky130.lib.spice tt
  ```
  If the `PDK_ROOT` environment variable is not set to match this path, `two_stage_opamp` fails immediately.
- `adc_sar` is self-contained (ideal DAC + comparator), so it is the better sanity check for a fresh environment.
- `r_ladder_dac_sky130` expects `PDK_ROOT` to point to a Sky130 PDK install; it uses the `res_xhigh_po` subcircuit (instantiate with `X` prefix, `l`/`w` in microns without a `u` suffix).
- The ngspice shared library is initialised once per process. `NgSpiceInterface` routes callbacks through the active instance so loops that create a fresh `AMSTestbench` per voltage/netlist do not call back into a destroyed object.
- There is no test runner, linter, or formatter configured. The examples are the only executable verification.
