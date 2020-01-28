module full_adder
	(input logic a,
	input logic b,
	input logic cin,
	output logic s,
	output logic cout
);

assign s = cin ^ a ^ b;
assign cout = a & b | b & cin | a & cin;

endmodule

module approx_full_adder
	(input logic a,
	input logic b,
	input logic cin,
	output logic s,
	output logic cout
);

assign s = a | b;
assign cout = '0;

endmodule

module approx_adder
	#(parameter APPROX_LV = 0)
	(
		input logic [31:0] a,
		input logic [31:0] b,
		output logic [31:0] sum,
		output logic		c
	);

	logic [31:0] sumt;
	logic [32:0] carries;

	assign carries[0] = '0;

	assign sum = sumt;
	assign c = carries[32];
	genvar i;
	generate 
		for (i = 0; i < APPROX_LV; i = i+1) begin
			approx_full_adder appx(
				.a(a[i]),
				.b(b[i]),
				.cin(carries[i]),
				.s(sumt[i]),
				.cout(carries[i+1]));
		end
		for (i = APPROX_LV; i < 32; i = i+1) begin
			full_adder fadd(
				.a(a[i]),
				.b(b[i]),
				.cin(carries[i]),
				.s(sumt[i]),
				.cout(carries[i+1]));
		end

	endgenerate

endmodule


