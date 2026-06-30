module square_wave_driver (
    input  wire        clk,
    input  wire        rst_n,
    input  wire        enable,      // active high enable
    input  wire [31:0] period,     // toggle period in clock cycles
    output reg         wave_out     // square wave output
);

reg [31:0] cycle_cnt;

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        wave_out  <= 1'b0;
        cycle_cnt <= 32'd0;
    end else if (enable) begin
        if (cycle_cnt >= period - 1) begin
            wave_out  <= ~wave_out;
            cycle_cnt <= 32'd0;
        end else begin
            cycle_cnt <= cycle_cnt + 32'd1;
        end
    end
end

endmodule