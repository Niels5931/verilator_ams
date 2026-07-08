module ams_top;
    import uvm_pkg::*;
    import ams_dpi_pkg::*;

    logic        clk;
    logic        rst_n;
    logic [31:0] vout;
    logic        b0;
    logic        b1;
    logic        b2;
    logic        b3;

    // Clock generation: 100 ns period from config.yaml.
    initial clk = 0;
    always #50 clk = ~clk;

    // Reset generation.
    initial begin
        rst_n = 0;
        repeat (5) @(posedge clk);
        rst_n = 1;
    end

    // AMS synchronization process.
    // At the negedge we sample stable digital outputs, drive them into ngspice
    // as voltage sources, advance ngspice by one clock period, and apply the
    // resulting analog input before the next posedge.
    always @(negedge clk) begin
        real vdd;
        vdd = ams_dpi_pkg::ams_get_vdd();
        ams_dpi_pkg::ams_set_voltage("b0", b0 ? vdd : 0.0);
        ams_dpi_pkg::ams_set_voltage("b1", b1 ? vdd : 0.0);
        ams_dpi_pkg::ams_set_voltage("b2", b2 ? vdd : 0.0);
        ams_dpi_pkg::ams_set_voltage("b3", b3 ? vdd : 0.0);
        ams_dpi_pkg::ams_run_analog(100.0);
        vout = int'(ams_dpi_pkg::ams_get_voltage("vout") * 65536.0);
    end

    // DUT instance.
    r_ladder_dac_dut u_dut (
        .clk   (clk),
        .rst_n (rst_n),
        .vout  (vout),
        .b0    (b0),
        .b1    (b1),
        .b2    (b2),
        .b3    (b3)
    );

    // Virtual interface for UVM testbench access.
    r_ladder_dac_if vif (clk, rst_n);
    assign vif.vout = vout;
    assign vif.b0   = b0;
    assign vif.b1   = b1;
    assign vif.b2   = b2;
    assign vif.b3   = b3;

    // UVM entry point.
    initial begin
        uvm_config_db#(virtual r_ladder_dac_if)::set(null, "*", "vif", vif);
        run_test("r_ladder_dac_test");
    end

endmodule
