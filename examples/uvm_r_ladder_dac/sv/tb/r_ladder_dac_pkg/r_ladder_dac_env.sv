class r_ladder_dac_env extends uvm_env;
  `uvm_component_utils(r_ladder_dac_env)

  r_ladder_dac_agent      agent;
  r_ladder_dac_scoreboard scoreboard;

  uvm_event analog_settled;

  function new(string name = "r_ladder_dac_env", uvm_component parent = null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    agent      = r_ladder_dac_agent::type_id::create("agent", this);
    scoreboard = r_ladder_dac_scoreboard::type_id::create("scoreboard", this);
    analog_settled = new("analog_settled");
  endfunction

  virtual function void connect_phase(uvm_phase phase);
    super.connect_phase(phase);

    // Share the analog-settled synchronizer with the driver and monitor by
    // direct handle (not via config_db).
    agent.driver.analog_settled  = analog_settled;
    agent.monitor.analog_settled = analog_settled;

    // The monitor publishes sampled transactions to both the scoreboard and
    // the coverage collector.
    agent.monitor.ap.connect(scoreboard.analysis_export);
    agent.monitor.ap.connect(agent.coverage.analysis_export);
  endfunction
endclass
