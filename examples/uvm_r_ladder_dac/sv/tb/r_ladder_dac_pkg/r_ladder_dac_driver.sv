class r_ladder_dac_driver extends uvm_driver#(r_ladder_dac_seq_item);
  `uvm_component_utils(r_ladder_dac_driver)

  virtual r_ladder_dac_if vif;
  uvm_event analog_settled;

  function new(string name = "r_ladder_dac_driver", uvm_component parent = null);
    super.new(name, parent);
  endfunction

  virtual function void build_phase(uvm_phase phase);
    super.build_phase(phase);
    if (!uvm_config_db#(virtual r_ladder_dac_if)::get(this, "", "vif", vif)) begin
      `uvm_fatal("DRIVER", "Failed to get virtual interface")
    end
  endfunction

  virtual task run_phase(uvm_phase phase);
    real vdd;
    real vout;
    vdd = ams_dpi_pkg::ams_get_vdd();

    forever begin
      r_ladder_dac_seq_item req;

      seq_item_port.get_next_item(req);
      wait(vif.rst_n == 1'b1);

      // Drive the new code at the rising edge.
      @(posedge vif.clk);
      vif.code = req.code;

      // Push the digital code into ngspice and advance the analog simulation
      // by one clock period.
      ams_dpi_pkg::ams_set_voltage("b0", req.code[0] ? vdd : 0.0);
      ams_dpi_pkg::ams_set_voltage("b1", req.code[1] ? vdd : 0.0);
      ams_dpi_pkg::ams_set_voltage("b2", req.code[2] ? vdd : 0.0);
      ams_dpi_pkg::ams_set_voltage("b3", req.code[3] ? vdd : 0.0);
      ams_dpi_pkg::ams_run_analog(ams_dpi_pkg::ams_get_clock_period_ns());

      // Sample the settled analog result and write it back to the interface
      // for VCD, then notify observers that the analog state is stable.
      vout = ams_dpi_pkg::ams_get_voltage("vout");
      `uvm_info("DRIVER", $sformatf("sampled vout=%f for code=%0d", vout, req.code), UVM_LOW)
      vif.vout = int'(vout * 65536.0);
      if (analog_settled != null) analog_settled.trigger();

      seq_item_port.item_done();
    end
  endtask
endclass
