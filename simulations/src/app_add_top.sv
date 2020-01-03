`include "defines.sv"

import defines::*;


module app_add_top(
    input                                          clk,
    input                                          reset,

    input logic[31:0]							   a,
    input logic[31:0]							   b,
	output logic[31:0]							   of_output_ori,
	output logic[31:0]							   of_output_app
);

	logic[32:0]	of_output_ori_temp;


	approx_adder #(.APPROX_LV(16)) apr16(
		.a(a),
		.b(b),
		.sum(of_output_app),
		.c()	// unconnected
	);

	assign of_output_ori_temp = a+b;
	assign of_output_ori = of_output_ori_temp[31:0];




endmodule

