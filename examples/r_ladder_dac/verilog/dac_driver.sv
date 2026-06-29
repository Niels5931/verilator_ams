module dac_driver (
    input  wire        clk,
    input  wire        rst_n,
    output reg  [3:0]  code
);

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            code <= 4'd0;
        end else begin
            code <= code + 4'd1;
        end
    end

endmodule
