`include "defines.sv"

import defines::*;


module simtop(
    input                                          clk,
    input                                          reset,

    input logic[31:0]							   of_operand1_1,
    input logic[31:0]							   of_operand1_2,
    input logic[31:0]							   of_operand1_3,
    input logic[31:0]							   of_operand1_4,
    input logic[31:0]							   of_operand2_1,
    input logic[31:0]							   of_operand2_2,
    input logic[31:0]							   of_operand2_3,
    input logic[31:0]							   of_operand2_4,
    input vector_lane_mask_t                       of_mask_value,
    input                                          of_instruction_valid,
    input logic[5:0]	                           of_instruction_i,
	input logic[1:0]							   of_pipeline_sel,
    input local_thread_idx_t                       of_thread_idx,
    input subcycle_t                               of_subcycle,

	output logic[31:0]							   of_output_1,
	output logic[31:0]							   of_output_2,
	output logic[31:0]							   of_output_3,
	output logic[31:0]							   of_output_4
);

	// local variants of the operands and inputs
    decoded_instruction_t of_instruction;
	vector_t of_operand1;
	vector_t of_operand2;

	// put the input instruciton and pipeline selection into the struct
	assign of_instruction.alu_op = of_instruction_i;
	assign of_instruction.pipeline_sel = of_pipeline_sel;

	// assign the output from the last stage to the real output
	assign of_output_1 = fx5_result[0];
	assign of_output_2 = fx5_result[1];
	assign of_output_3 = fx5_result[2];
	assign of_output_4 = fx5_result[3];

	// generate to copy the 4 input values along the 16 lanes
	// this is done for operand1 and operand2
	genvar i;
	generate
		for (i = 0; i < 4; i++) begin
			assign of_operand1[i*4+0] = of_operand1_1; 
			assign of_operand1[i*4+1] = of_operand1_2; 
			assign of_operand1[i*4+2] = of_operand1_3; 
			assign of_operand1[i*4+3] = of_operand1_4; 

			assign of_operand2[i*4+0] = of_operand2_1; 
			assign of_operand2[i*4+1] = of_operand2_2; 
			assign of_operand2[i*4+2] = of_operand2_3; 
			assign of_operand2[i*4+3] = of_operand2_4; 
			
		end
	endgenerate

	//
	// from here on out nyuzi specific stuff
	//



    logic [NUM_VECTOR_LANES-1:0] [7:0] fx1_add_exponent;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] fx1_add_result_sign;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [5:0] fx1_ftoi_lshift;// From fp_execute_stage1 of fp_execute_stage1.v
    decoded_instruction_t fx1_instruction;      // From fp_execute_stage1 of fp_execute_stage1.v
    logic               fx1_instruction_valid;  // From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] fx1_logical_subtract;// From fp_execute_stage1 of fp_execute_stage1.v
    vector_lane_mask_t  fx1_mask_value;         // From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx1_mul_exponent;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] fx1_mul_sign;  // From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [31:0] fx1_multiplicand;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [31:0] fx1_multiplier;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] fx1_result_is_inf;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] fx1_result_is_nan;// From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [5:0] fx1_se_align_shift;// From fp_execute_stage1 of fp_execute_stage1.v
    scalar_t [NUM_VECTOR_LANES-1:0] fx1_significand_le;// From fp_execute_stage1 of fp_execute_stage1.v
    scalar_t [NUM_VECTOR_LANES-1:0] fx1_significand_se;// From fp_execute_stage1 of fp_execute_stage1.v
    subcycle_t          fx1_subcycle;           // From fp_execute_stage1 of fp_execute_stage1.v
    local_thread_idx_t  fx1_thread_idx;         // From fp_execute_stage1 of fp_execute_stage1.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx2_add_exponent;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_add_result_sign;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] [5:0] fx2_ftoi_lshift;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_guard;     // From fp_execute_stage2 of fp_execute_stage2.v
    decoded_instruction_t fx2_instruction;      // From fp_execute_stage2 of fp_execute_stage2.v
    logic               fx2_instruction_valid;  // From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_logical_subtract;// From fp_execute_stage2 of fp_execute_stage2.v
    vector_lane_mask_t  fx2_mask_value;         // From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx2_mul_exponent;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_mul_sign;  // From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_result_is_inf;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_result_is_nan;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_round;     // From fp_execute_stage2 of fp_execute_stage2.v
    scalar_t [NUM_VECTOR_LANES-1:0] fx2_significand_le;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] [63:0] fx2_significand_product;// From fp_execute_stage2 of fp_execute_stage2.v
    scalar_t [NUM_VECTOR_LANES-1:0] fx2_significand_se;// From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] fx2_sticky;    // From fp_execute_stage2 of fp_execute_stage2.v
    subcycle_t          fx2_subcycle;           // From fp_execute_stage2 of fp_execute_stage2.v
    local_thread_idx_t  fx2_thread_idx;         // From fp_execute_stage2 of fp_execute_stage2.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx3_add_exponent;// From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] fx3_add_result_sign;// From fp_execute_stage3 of fp_execute_stage3.v
    scalar_t [NUM_VECTOR_LANES-1:0] fx3_add_significand;// From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] [5:0] fx3_ftoi_lshift;// From fp_execute_stage3 of fp_execute_stage3.v
    decoded_instruction_t fx3_instruction;      // From fp_execute_stage3 of fp_execute_stage3.v
    logic               fx3_instruction_valid;  // From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] fx3_logical_subtract;// From fp_execute_stage3 of fp_execute_stage3.v
    vector_lane_mask_t  fx3_mask_value;         // From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx3_mul_exponent;// From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] fx3_mul_sign;  // From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] fx3_result_is_inf;// From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] fx3_result_is_nan;// From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] [63:0] fx3_significand_product;// From fp_execute_stage3 of fp_execute_stage3.v
    subcycle_t          fx3_subcycle;           // From fp_execute_stage3 of fp_execute_stage3.v
    local_thread_idx_t  fx3_thread_idx;         // From fp_execute_stage3 of fp_execute_stage3.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx4_add_exponent;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] fx4_add_result_sign;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] [31:0] fx4_add_significand;// From fp_execute_stage4 of fp_execute_stage4.v
    decoded_instruction_t fx4_instruction;      // From fp_execute_stage4 of fp_execute_stage4.v
    logic               fx4_instruction_valid;  // From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] fx4_logical_subtract;// From fp_execute_stage4 of fp_execute_stage4.v
    vector_lane_mask_t  fx4_mask_value;         // From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] [7:0] fx4_mul_exponent;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] fx4_mul_sign;  // From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] [5:0] fx4_norm_shift;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] fx4_result_is_inf;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] fx4_result_is_nan;// From fp_execute_stage4 of fp_execute_stage4.v
    logic [NUM_VECTOR_LANES-1:0] [63:0] fx4_significand_product;// From fp_execute_stage4 of fp_execute_stage4.v
    subcycle_t          fx4_subcycle;           // From fp_execute_stage4 of fp_execute_stage4.v
    local_thread_idx_t  fx4_thread_idx;         // From fp_execute_stage4 of fp_execute_stage4.v
    decoded_instruction_t fx5_instruction;      // From fp_execute_stage5 of fp_execute_stage5.v
    logic               fx5_instruction_valid;  // From fp_execute_stage5 of fp_execute_stage5.v
    vector_lane_mask_t  fx5_mask_value;         // From fp_execute_stage5 of fp_execute_stage5.v
    vector_t            fx5_result;             // From fp_execute_stage5 of fp_execute_stage5.v
    subcycle_t          fx5_subcycle;           // From fp_execute_stage5 of fp_execute_stage5.v
    local_thread_idx_t  fx5_thread_idx;         // From fp_execute_stage5 of fp_execute_stage5.v

    decoded_instruction_t ix_instruction;       // From int_execute_stage of int_execute_stage.v
    logic               ix_instruction_valid;   // From int_execute_stage of int_execute_stage.v
    logic               ix_is_eret;             // From int_execute_stage of int_execute_stage.v
    vector_lane_mask_t  ix_mask_value;          // From int_execute_stage of int_execute_stage.v
    logic               ix_perf_cond_branch_not_taken;// From int_execute_stage of int_execute_stage.v
    logic               ix_perf_cond_branch_taken;// From int_execute_stage of int_execute_stage.v
    logic               ix_perf_uncond_branch;  // From int_execute_stage of int_execute_stage.v
    logic               ix_privileged_op_fault; // From int_execute_stage of int_execute_stage.v
    vector_t            ix_result;              // From int_execute_stage of int_execute_stage.v
    logic               ix_rollback_en;         // From int_execute_stage of int_execute_stage.v
    scalar_t            ix_rollback_pc;         // From int_execute_stage of int_execute_stage.v
    subcycle_t          ix_subcycle;            // From int_execute_stage of int_execute_stage.v
    local_thread_idx_t  ix_thread_idx;          // From int_execute_stage of int_execute_stage.v

    logic               wb_perf_instruction_retire;// From writeback_stage of writeback_stage.v
    logic               wb_perf_store_rollback; // From writeback_stage of writeback_stage.v
    logic               wb_rollback_en;         // From writeback_stage of writeback_stage.v
    scalar_t            wb_rollback_pc;         // From writeback_stage of writeback_stage.v
    pipeline_sel_t      wb_rollback_pipeline;   // From writeback_stage of writeback_stage.v
    subcycle_t          wb_rollback_subcycle;   // From writeback_stage of writeback_stage.v
    local_thread_idx_t  wb_rollback_thread_idx; // From writeback_stage of writeback_stage.v
    local_thread_bitmap_t wb_suspend_thread_oh; // From writeback_stage of writeback_stage.v
    logic               wb_trap;                // From writeback_stage of writeback_stage.v
    scalar_t            wb_trap_access_vaddr;   // From writeback_stage of writeback_stage.v
    trap_cause_t        wb_trap_cause;          // From writeback_stage of writeback_stage.v
    scalar_t            wb_trap_pc;             // From writeback_stage of writeback_stage.v
    subcycle_t          wb_trap_subcycle;       // From writeback_stage of writeback_stage.v
    local_thread_idx_t  wb_trap_thread_idx;     // From writeback_stage of writeback_stage.v
    logic               wb_writeback_en;        // From writeback_stage of writeback_stage.v
    logic               wb_writeback_is_last_subcycle;// From writeback_stage of writeback_stage.v
    logic               wb_writeback_is_vector; // From writeback_stage of writeback_stage.v
    vector_lane_mask_t  wb_writeback_mask;      // From writeback_stage of writeback_stage.v
    register_idx_t      wb_writeback_reg;       // From writeback_stage of writeback_stage.v
    local_thread_idx_t  wb_writeback_thread_idx;// From writeback_stage of writeback_stage.v
    vector_t            wb_writeback_value;     // From writeback_stage of writeback_stage.v

//    int_execute_stage int_execute_stage(.*);
    fp_execute_stage1 fp_execute_stage1(.*);
    fp_execute_stage2 fp_execute_stage2(.*);
    fp_execute_stage3 fp_execute_stage3(.*);
    fp_execute_stage4 fp_execute_stage4(.*);
    fp_execute_stage5 fp_execute_stage5(.*);

endmodule

