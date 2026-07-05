module r_ladder_dac_dut (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [31:0] vout,
    output wire        b0,
    output wire        b1,
    output wire        b2,
    output wire        b3
);

    wire [3:0] code;

    dac_driver u_dac_driver (
        .clk   (clk),
        .rst_n (rst_n),
        .code  (code)
    );

    assign b0 = code[0];
    assign b1 = code[1];
    assign b2 = code[2];
    assign b3 = code[3];

endmodule
