module ams_top (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [31:0] analog_in,       // Q16.16 fixed-point voltage from ngspice
    input  wire        analog_in_valid,  // high when analog_in has new data
    output wire        digital_out       // 1-bit digital output to ngspice
);

localparam HALF_PERIOD = 32'd2;

square_wave_driver u_square_wave (
    .clk      (clk),
    .rst_n    (rst_n),
    .enable   (1'b1),
    .period   (HALF_PERIOD),
    .wave_out (digital_out)
);

endmodule