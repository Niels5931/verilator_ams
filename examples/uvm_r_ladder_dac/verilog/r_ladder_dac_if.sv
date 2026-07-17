interface r_ladder_dac_if (
    input logic clk,
    input logic rst_n
);
    logic [31:0] vout;
    logic [3:0]  code;
    logic        b0;
    logic        b1;
    logic        b2;
    logic        b3;
endinterface
