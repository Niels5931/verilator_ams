class r_ladder_dac_agent extends uvm_agent;
  `uvm_component_utils(r_ladder_dac_agent)

  r_ladder_dac_sequencer sequencer;
  r_ladder_dac_driver    driver;
  r_ladder_dac_monitor   monitor;
  r_ladder_dac_coverage  coverage;

  function new(string name = "r_ladder_dac_agent", uvm_component parent = null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    sequencer = r_ladder_dac_sequencer::type_id::create("sequencer", this);
    driver    = r_ladder_dac_driver::type_id::create("driver", this);
    monitor   = r_ladder_dac_monitor::type_id::create("monitor", this);
    coverage  = r_ladder_dac_coverage::type_id::create("coverage", this);
  endfunction

  virtual function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);
    driver.seq_item_port.connect(sequencer.seq_item_export);
  endfunction
endclass
