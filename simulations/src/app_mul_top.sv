`include "defines.sv"

import defines::*;

// topmodule for the simulation testbench
module app_mul_top(
    input                                          clk,
    input                                          reset,

    input logic[31:0]							   a,
    input logic[31:0]							   b,
	output logic[31:0]							   of_output_ori,
	output logic[31:0]							   of_output_app
);

// reduce the inputs for beauty
logic [15:0] a_red, b_red;
assign a_red = a[15:0];
assign b_red = b[15:0];

// mult with whatever synthesis comes up
assign of_output_ori = a*b;

// the mitchell multiplier
mitchell_mult mitchi (
	.sign(sign),
	.clk(clk),
	.reset(reset),
	.multiplicant(a_red),
	.multiplier(b_red),
	.product(of_output_app)
);



endmodule;
