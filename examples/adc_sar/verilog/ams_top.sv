module ams_top (
    input  wire        clk,
    input  wire        rst_n,
    input  wire        cmp_in,        // comparator output from ngspice (1-bit A/D)
    output wire [3:0]  dac_code,      // 4-bit DAC code to ngspice (D/A)
    output wire [3:0]  adc_code,      // final 4-bit conversion result
    output wire        done           // high for one cycle when adc_code is valid
);

reg start;

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        start <= 1'b1;
    end else begin
        start <= done;
    end
end

sar_adc_controller #(
    .WIDTH(4)
) u_sar (
    .clk      (clk),
    .rst_n    (rst_n),
    .cmp_in   (cmp_in),
    .start    (start),
    .dac_code (dac_code),
    .adc_code (adc_code),
    .done     (done)
);

endmodule
