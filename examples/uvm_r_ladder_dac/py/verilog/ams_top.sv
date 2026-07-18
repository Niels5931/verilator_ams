module ams_top;
    logic        clk;
    logic        rst_n;
    logic [3:0]  code;
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

    // DUT instance.
    r_ladder_dac_dut u_dut (
        .code (code),
        .b0   (b0),
        .b1   (b1),
        .b2   (b2),
        .b3   (b3)
    );

endmodule
