# AMS Co-Simulation with Verilator + ngspice

> **⚠️ Vibe-coded disclaimer:** This project was built interactively with AI assistance ("vibe coding"). It is functional for demonstration purposes, but expect rough edges, incomplete features, and spots that need engineering review before production use. Treat it as a starting point, not a finished product.

This repo is a work-in-progress C++17 CMake project that co-simulates Verilog (via Verilator) with analog SPICE netlists (via ngspice) in a single process with lockstep synchronization per clock cycle.

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

Requires CMake ≥ 3.19, C++17, Verilator, and the ngspice shared dev library (`libngspice`/`ngspice-0` + `sharedspice.h`). CMake fetches `yaml-cpp` automatically on first configure.

```bash
cmake -S . -B build
cmake --build build
```

## Run an example

Each example has a helper script:

```bash
./examples/diff_pair/run.sh
./examples/adc_sar/run.sh
```

Or manually:

```bash
cmake --build build --target ams_sim_diff_pair
cd examples/diff_pair
../../build/examples/diff_pair/ams_sim_diff_pair config.yaml
```

Run from the example directory because `spice_netlist` paths in `config.yaml` are relative.

## Examples

- **`diff_pair`** — Sky130 differential pair driven by a digital square wave. Reads the differential output back as a Q16.16 voltage.
- **`adc_sar`** — 4-bit SAR ADC co-simulation with an ideal behavioral DAC + comparator in ngspice. **Experimental:** it builds and runs end-to-end, but the conversion result is not yet correct and needs further debugging.

## General expectations

- The SPICE netlist must contain a `.tran` line with a total run time.
- ngspice is stepped by exactly one Verilator clock period per `step()` call using `bg_run` / `bg_resume` breakpoints.
- The `.tran` end time only needs to be longer than the total simulated time; the co-simulation halts at each clock boundary.
- Digital inputs to ngspice are held stable for the full clock period.
- Analog outputs are sampled at the clock boundary.

## Wiring a new co-simulation

See `examples/*/main.cpp`:

1. Load the config: `ams::loadConfig(argv[1])`.
2. Instantiate `ams::AMSTestbench<Vtop>(config)`.
3. Register port accessors/writers **before** `init()`:
   - `tb.output_accessors_["verilog_port"] = [](const Vtop* d) -> uint32_t { ... };`
   - `tb.input_writers_["verilog_port"] = [](Vtop* d, uint32_t val) { ... };`
4. Call `tb.init(argc, argv)` to load the SPICE netlist and reset the DUT.
5. Call `tb.openTrace("ams_waveforms.vcd")` before stepping to capture a VCD trace.
6. Drive the loop with `tb.step()` or `tb.run(n_cycles)`.

`init()` also accepts an optional `std::vector<std::string>` netlist if you need to build or modify the SPICE text in C++ (see `adc_sar`).
