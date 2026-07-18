class r_ladder_dac_monitor extends uvm_monitor;
  `uvm_component_utils(r_ladder_dac_monitor)

  virtual r_ladder_dac_if vif;
  uvm_analysis_port#(r_ladder_dac_result_item) ap;
  uvm_event analog_settled;

  function new(string name = "r_ladder_dac_monitor", uvm_component parent = null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual r_ladder_dac_if)::get(this, "", "vif", vif)) begin
      `uvm_fatal("MONITOR", "Failed to get virtual interface")
    end
    ap = new("ap", this);
  endfunction

  virtual task run_phase(uvm_phase phase);
    r_ladder_dac_result_item tr;

    forever begin
      // Wait until the driver signals that ngspice has settled, then sample
      // the interface proxy values. Sampling at the event (rather than free-
      // running on the clock) avoids racing the driver's vif.vout write.
      analog_settled.wait_trigger();

      tr       = r_ladder_dac_result_item::type_id::create("tr");
      tr.code  = vif.code;
      tr.vout  = real'(vif.vout) / 65536.0;
      `uvm_info("MONITOR", $sformatf("sampled vif.code=%0d vif.vout=%0d -> vout=%f", vif.code, vif.vout, tr.vout), UVM_LOW)

      ap.write(tr);
    end
  endtask
endclass
