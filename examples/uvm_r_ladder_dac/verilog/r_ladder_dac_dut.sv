module r_ladder_dac_dut (
    input  wire [3:0] code,
    output wire       b0,
    output wire       b1,
    output wire       b2,
    output wire       b3
);

    assign b0 = code[0];
    assign b1 = code[1];
    assign b2 = code[2];
    assign b3 = code[3];

endmodule
