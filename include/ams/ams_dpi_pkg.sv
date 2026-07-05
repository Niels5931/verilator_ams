package ams_dpi_pkg;
  // Lifecycle
  import "DPI-C" function int  ams_init        (string config_path);
  import "DPI-C" function int  ams_finish      ();

  // Fine-grained analog synchronization.  The UVM testbench composes these
  // with its own clocking/reset agents.
  import "DPI-C" function int  ams_sync_d2a    ();           // DUT outputs -> ngspice sources
  import "DPI-C" function int  ams_run_analog  (real dt_ns); // advance ngspice by dt_ns
  import "DPI-C" function int  ams_sync_a2d    ();           // ngspice nodes -> DUT inputs
  import "DPI-C" function int  ams_sync        (real dt_ns); // d2a + run + a2d convenience

  // Direct raw access to arbitrary ngspice nodes/sources.
  import "DPI-C" function real ams_get_voltage (string spice_node);
  import "DPI-C" function int  ams_set_voltage (string spice_source, real voltage);

  // VCD trace control
  import "DPI-C" function int  ams_open_trace  (string filename);
  import "DPI-C" function int  ams_close_trace ();

  // Configuration queries
  import "DPI-C" function real ams_get_vdd             ();
  import "DPI-C" function real ams_get_clock_period_ns ();
  import "DPI-C" function real ams_get_sim_time_ns     ();
endpackage
