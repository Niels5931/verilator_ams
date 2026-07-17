import uvm_pkg::*;
import ams_dpi_pkg::*;
`include "uvm_macros.svh"
import r_ladder_dac_pkg::*;

class r_ladder_dac_test extends uvm_test;
    `uvm_component_utils(r_ladder_dac_test)

    virtual r_ladder_dac_if vif;
    r_ladder_dac_env env;

    function new(string name = "r_ladder_dac_test", uvm_component parent = null);
        super.new(name, parent);
    endfunction

    virtual function void build_phase(uvm_phase phase);
        super.build_phase(phase);
        if (!uvm_config_db#(virtual r_ladder_dac_if)::get(this, "", "vif", vif)) begin
            `uvm_fatal("TEST", "Failed to get virtual interface")
        end
        env = r_ladder_dac_env::type_id::create("env", this);
    endfunction

    virtual function void start_of_simulation_phase(uvm_phase phase);
        super.start_of_simulation_phase(phase);
        ams_dpi_pkg::ams_init("config.yaml");
        ams_dpi_pkg::ams_open_trace("ams_waveforms.vcd");
    endfunction

    virtual task run_phase(uvm_phase phase);
        r_ladder_dac_ramp_sequence seq;

        phase.raise_objection(this);

        `uvm_info("TEST", $sformatf("VDD = %f", ams_dpi_pkg::ams_get_vdd()), UVM_LOW)

        seq = r_ladder_dac_ramp_sequence::type_id::create("seq");
        seq.start(env.agent.sequencer);

        // Allow the monitor and coverage collector to finish with the last
        // transaction before shutting down the AMS session.
        repeat (3) @(posedge vif.clk);

        ams_dpi_pkg::ams_close_trace();
        ams_dpi_pkg::ams_finish();

        phase.drop_objection(this);
    endtask
endclass
