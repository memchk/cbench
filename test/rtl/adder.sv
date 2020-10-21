module adder(
        input logic clk,
        input logic clk_offset,
        input logic [7:0] a,
        input logic [7:0] b,
        output logic [8:0] c,
        output logic [8:0] c_offset
);

always_ff @(posedge clk) begin
    c <= a + b;
end

always_ff @(posedge clk_offset) begin
    c_offset <= a + b;
end

endmodule