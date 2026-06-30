module sar_adc_controller #(
    parameter WIDTH = 4
) (
    input  wire             clk,
    input  wire             rst_n,
    input  wire             cmp_in,
    input  wire             start,
    output reg  [WIDTH-1:0] dac_code,
    output reg  [WIDTH-1:0] adc_code,
    output reg              done
);

localparam IDLE   = 2'd0;
localparam CONV   = 2'd1;
localparam FINISH = 2'd2;

localparam BIT_IDX_W = $clog2(WIDTH);
localparam [BIT_IDX_W-1:0] START_BIT = BIT_IDX_W'(WIDTH - 1);

reg [1:0] state;
reg [BIT_IDX_W-1:0] bit_idx;

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state    <= IDLE;
        bit_idx  <= START_BIT;
        dac_code <= {1'b1, {(WIDTH-1){1'b0}}};
        adc_code <= '0;
        done     <= 1'b0;
    end else begin
        done <= 1'b0;
        case (state)
            IDLE: begin
                if (start) begin
                    state    <= CONV;
                    bit_idx  <= START_BIT;
                    dac_code <= {1'b1, {(WIDTH-1){1'b0}}};
                end
            end

            CONV: begin
                dac_code[bit_idx] <= cmp_in;

                if (bit_idx == 0) begin
                    state <= FINISH;
                end else begin
                    bit_idx             <= bit_idx - 1;
                    dac_code[bit_idx-1] <= 1'b1;
                end
            end

            FINISH: begin
                adc_code <= dac_code;
                done     <= 1'b1;
                state    <= IDLE;
            end

            default: state <= IDLE;
        endcase
    end
end

endmodule
