`include "defines.svh"

import defines::*;

module app_mul_top(
	input sign,
	input logic[15:0] multiplicant,
	input logic[15:0] multiplier,
	output scalar_t product);
	
    logic [4:0] lod_multiplicant;
	logic [4:0] lod_multiplier;
	
	logic [14:0] frac_multiplicant;
	logic [14:0] frac_multiplier;
		
	logic [5:0] characteristic_sum;
	logic [15:0] fractional_sum_carry;
	logic [14:0] fractional_sum;
	logic [15:0] fractional_sum_corr_carry;
	logic [16:0] fractional_sum_appended;
	
	logic [47:0] internal_product;
	logic [30:0] shiftReg_multiplicant_tmp;
	logic [22:0] shiftReg_multiplicant;
    logic [30:0] shiftReg_multiplier_tmp;
    logic [22:0] shiftReg_multiplier;
	
	const logic [14:0] corr_factor = 15'b000101000000000;
	
	assign product = internal_product[47:16];
	
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
                    
                    default: lod_multiplier = 0;
                endcase
    // Barrel shifter for multiplicant's fractional part.
            shiftReg_multiplicant_tmp = {multiplicant, 15'b0} >> lod_multiplicant;
			shiftReg_multiplicant = shiftReg_multiplicant_tmp[22:0];
            frac_multiplicant = shiftReg_multiplicant[14:0];
		
	        // Barrel shifter for multipliers's fractional part.
		    shiftReg_multiplier_tmp = {multiplier, 15'b0} >> lod_multiplier;
		    shiftReg_multiplier = shiftReg_multiplier_tmp[22:0];
			frac_multiplier = shiftReg_multiplier[14:0];
	
            // Generate sum of fractional parts.
			fractional_sum_carry = frac_multiplicant + frac_multiplier;
			fractional_sum = fractional_sum_carry[14:0];
			if(fractional_sum_carry[15]) begin
			     fractional_sum_corr_carry = {'0,fractional_sum} + ({'0,corr_factor} >> 1);
			     if (fractional_sum_corr_carry[15]) begin
			         fractional_sum_appended = { 2'b10, fractional_sum_corr_carry[14:0] };
			     end
			     else begin
                    fractional_sum_appended = { 2'b01, fractional_sum_corr_carry[14:0] };
			     end
			 end
			else begin
			     fractional_sum_corr_carry = {'0,fractional_sum} + {'0,corr_factor};
			     if (fractional_sum_corr_carry[15]) begin
			         fractional_sum_appended = { 2'b10, fractional_sum_corr_carry[14:0] };
			     end
			     else begin
                    fractional_sum_appended = { 2'b01, fractional_sum_corr_carry[14:0] };
			     end
			end
			
	        // Generate sum of characteristic parts.
			characteristic_sum = {'0,lod_multiplicant} + {'0,lod_multiplier} + {4'b0,fractional_sum_carry[15]};
	
            // Final barrel shifter.
            internal_product = {31'b0,fractional_sum_appended};
            internal_product = internal_product << characteristic_sum; 
            
            // Handle sign flag.
            // If signs of multiplier and multiplicant are unequal, the product has to be negative.
            // Therefore, the already calculated product, has to be negated (this equals to building
            // a 2-complement). 
            if(sign && (multiplicant[15] != multiplier[15])) begin
                internal_product = (~internal_product) + 1'b1; 
            end
        end
endmodule
