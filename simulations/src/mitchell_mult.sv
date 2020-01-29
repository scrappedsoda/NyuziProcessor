`include "defines.sv"

import defines::*;

module mitchell_mult(
	input sign,
	input clk,
	input reset,
	input logic[15:0] multiplicant,
	input logic[15:0] multiplier,

	output scalar_t product
);
	
	integer lod_multiplicant;
	integer lod_multiplier;
	
	logic [31:0] frac_multiplicant;
	logic [31:0] frac_multiplier;
	
	logic result_zero_A;
	logic result_zero_B;
	
	integer characteristic_sum;
	logic [14:0] fractional_sum;
	
	
	
	// Leading one detector for multiplicant 
	always_comb
            begin
                casez (multiplicant)
                    16'b1???????????????: lod_multiplicant = 15;
                    16'b01??????????????: lod_multiplicant = 14;
                    16'b001?????????????: lod_multiplicant = 13;
                    16'b0001????????????: lod_multiplicant = 12;
                    16'b00001???????????: lod_multiplicant = 11;
                    16'b000001??????????: lod_multiplicant = 10;
                    16'b0000001?????????: lod_multiplicant = 9;
                    16'b00000001????????: lod_multiplicant = 8;
                    16'b000000001???????: lod_multiplicant = 7;
                    16'b0000000001??????: lod_multiplicant = 6;
                    16'b00000000001?????: lod_multiplicant = 5;
                    16'b000000000001????: lod_multiplicant = 4;
                    16'b0000000000001???: lod_multiplicant = 3;
                    16'b00000000000001??: lod_multiplicant = 2;
                    16'b000000000000001?: lod_multiplicant = 1;
                    16'b0000000000000001: lod_multiplicant = 0;
                    16'b0000000000000000: lod_multiplicant = 0;
                    
                    default: lod_multiplicant = 0;
                endcase
            end
	
	// Leading one detector for multiplier.
	always_comb
            begin
                casez (multiplier)
                    16'b1???????????????: lod_multiplier = 15;
                    16'b01??????????????: lod_multiplier = 14;
                    16'b001?????????????: lod_multiplier = 13;
                    16'b0001????????????: lod_multiplier = 12;
                    16'b00001???????????: lod_multiplier = 11;
                    16'b000001??????????: lod_multiplier = 10;
                    16'b0000001?????????: lod_multiplier = 9;
                    16'b00000001????????: lod_multiplier = 8;
                    16'b000000001???????: lod_multiplier = 7;
                    16'b0000000001??????: lod_multiplier = 6;
                    16'b00000000001?????: lod_multiplier = 5;
                    16'b000000000001????: lod_multiplier = 4;
                    16'b0000000000001???: lod_multiplier = 3;
                    16'b00000000000001??: lod_multiplier = 2;
                    16'b000000000000001?: lod_multiplier = 1;
                    16'b0000000000000001: lod_multiplier = 0;
                    16'b0000000000000000: lod_multiplier = 0;
                    
                    default: lod_multiplicant = 0;
                endcase
            end
	
	// Barrel shifter for multiplicant's fractional part.
	always_comb
		begin
			frac_multiplicant = {multiplicant, 16'b0} >> lod_multiplicant;
		end
		
	// Barrel shifter for multipliers's fractional part.
	always_comb
		begin
			frac_multiplier = {multiplier, 16'b0} >> lod_multiplier;
		end
	
	// Generate sum of characteristic parts.
	always_comb
		begin
			characteristic_sum = lod_multiplicant + lod_multiplier;
		end
	
	// Generate sum of fractional parts.
	always_comb
		begin
			fractional_sum = frac_multiplicant[15:1] + frac_multiplier[15:1];
		end
	
	// Final barrel shifter.
	always_comb
		begin
			// expand fractional sum to 32 bit for correct shifting
			product = {17'b0,{fractional_sum}} << characteristic_sum;
		end
	
endmodule;
