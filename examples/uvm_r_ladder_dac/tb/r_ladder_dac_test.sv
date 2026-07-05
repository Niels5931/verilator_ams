import uvm_pkg::*;
`include "uvm_macros.svh"

class r_ladder_dac_test extends uvm_test;
    `uvm_component_utils(r_ladder_dac_test)

    virtual r_ladder_dac_if vif;

    function new(string name = "r_ladder_dac_test", uvm_component parent = null);
        super.new(name, parent);
    endfunction

    virtual function void build_phase(uvm_phase phase);
        super.build_phase(phase);
        if (!uvm_config_db#(virtual r_ladder_dac_if)::get(this, "", "vif", vif)) begin
            `uvm_fatal("TEST", "Failed to get virtual interface")
        end
    endfunction

    virtual task run_phase(uvm_phase phase);
        real vout;
        int  code;
        real expected;
        real vdd;

        phase.raise_objection(this);

        // Initialize the AMS co-simulation.
        ams_dpi_pkg::ams_init("config.yaml");
        ams_dpi_pkg::ams_open_trace("ams_waveforms.vcd");

        vdd = ams_dpi_pkg::ams_get_vdd();
        `uvm_info("TEST", $sformatf("VDD = %f", vdd), UVM_LOW)

        // Wait for reset to de-assert.
        @(posedge vif.clk iff vif.rst_n);

        // Run through all 16 DAC codes.
        for (int i = 0; i < 16; i++) begin
            @(posedge vif.clk);
            code = {vif.b3, vif.b2, vif.b1, vif.b0};
            vout = ams_dpi_pkg::ams_get_voltage("vout");
            expected = vdd * code / 16.0;
            `uvm_info("TEST",
                $sformatf("code=%0d vout=%f expected=%f", code, vout, expected),
                UVM_LOW)
        end

        ams_dpi_pkg::ams_close_trace();
        ams_dpi_pkg::ams_finish();

        phase.drop_objection(this);
    endtask

endclass
