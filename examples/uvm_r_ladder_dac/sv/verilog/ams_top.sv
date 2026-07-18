module ams_top;
    import uvm_pkg::*;

    logic clk;
    logic rst_n;

    // Clock generation: 100 ns period from config.yaml.
    initial clk = 0;
    always #50 clk = ~clk;

    // Reset generation.
    initial begin
        rst_n = 0;
        repeat (5) @(posedge clk);
        rst_n = 1;
    end

    // Virtual interface for UVM testbench access.  The driver writes vif.code
    // and vif.vout; the DUT drives vif.b0..b3 from vif.code.
    r_ladder_dac_if vif (clk, rst_n);

    // DUT instance.
    r_ladder_dac_dut u_dut (
        .code (vif.code),
        .b0   (vif.b0),
        .b1   (vif.b1),
        .b2   (vif.b2),
        .b3   (vif.b3)
    );

    // UVM entry point.
    initial begin
        uvm_config_db#(virtual r_ladder_dac_if)::set(null, "*", "vif", vif);
        run_test("r_ladder_dac_test");
    end

endmodule
