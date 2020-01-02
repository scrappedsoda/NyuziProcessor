#include <verilated.h>          // Defines common routines
#include "Vsimtop.h"

#include "verilated_vcd_c.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

#include <ctime>
#include <iomanip>

#define TESTLENMUL 5

const float TESTVALSA[20] = {\
 42.59416542, 91.5413809, 14.46178181, 67.95095313, 15.24479533, 60.72847429, 17.86065537, 19.77739195, 58.64516795, 94.54978956, 20.88424068, 67.13416854, 53.96558114, 21.95704399, 53.50537341, 58.15778596, 58.67270054, 36.77368986, 30.81642376, 87.25916665};
const float TESTVALSB[20] = {\
68.92367876, 30.35753474, 24.22455184,  2.96891913, 55.41379571, 61.09126623,\
18.48708651, 62.65560042, 15.24060724, 54.94527689, 26.09372454, 80.09397995,\
54.73872783,  0.68704942, 30.28776541, 79.04484901, 62.7460863,  12.73831338,\
34.76286231, 29.21613099};
const float TESTVALSSOL[20] = {\
 2935.74657456, 2778.97065044,  350.33018326,  201.74088445,   844.7719741,\
 3709.97939068,  330.19148093, 1239.16436715,  893.7879712,  5195.06436695,\
  544.94762357, 5377.04274892, 2954.00725832,   15.08557441, 1620.55819777,\
 4597.07341006, 3681.48233139,  468.43478542, 1071.26709608,  2549.3752428};

const float TESTVALSSOL2[20] = {\
111.51784418, 121.89891563,  38.68633366,  70.91987225,
70.65859104, 121.81974052,  36.34774188,  82.43299237,
73.88577519, 149.49506644,  46.97796522, 147.22814849,
108.70430897,  22.64409341,  83.79313881, 137.20263497,
121.41878684,  49.51200324,  65.57928607, 116.47529764};


enum instructions {
    OP_OR                   = 0 ,    // Bitwise logical or
    OP_AND                  = 1 ,
    OP_XOR                  = 3 ,
    OP_ADD_I                = 5 ,    // Add integer
    OP_SUB_I                = 6 ,    // Subtract integer
    OP_MULL_I               = 7 ,    // Multiply integer low
    OP_MULH_U               = 8 ,    // Unsigned multiply, return high bits
    OP_ASHR                 = 9 ,    // Arithmetic shift right (sign extend)
    OP_SHR                  = 10,    // Logical shift right (no sign extend)
    OP_SHL                  = 11,    // Logical shift left
    OP_CLZ                  = 12,    // Count leading zeroes
    OP_SHUFFLE              = 13,    // Shuffle vector lanes
    OP_CTZ                  = 14,    // Count trailing zeroes
    OP_MOVE                 = 15,
    OP_CMPEQ_I              = 16,    // Integer equal
    OP_CMPNE_I              = 17,    // Integer not equal
    OP_CMPGT_I              = 18,    // Integer greater (signed)
    OP_CMPGE_I              = 19,    // Integer greater or equal (signed)
    OP_CMPLT_I              = 20,    // Integer less than (signed)
    OP_CMPLE_I              = 21,    // Integer less than or equal (signed)
    OP_CMPGT_U              = 22,    // Integer greater than (unsigned)
    OP_CMPGE_U              = 23,    // Integer greater or equal (unsigned)
    OP_CMPLT_U              = 24,    // Integer less than (unsigned)
    OP_CMPLE_U              = 25,    // Integer less than or equal (unsigned)
    OP_GETLANE              = 26,    // Getlane
    OP_FTOI                 = 27,    // Float to integer
    OP_RECIPROCAL           = 28,    // Reciprocal estimate
    OP_SEXT8                = 29,    // Sign extend 8 bit value
    OP_SEXT16               = 30,    // Sign extend 16 bit value
    OP_MULH_I               = 31,    // Signed multiply high
    OP_ADD_F                = 32,    // Add floating point
    OP_SUB_F                = 33,    // Subtract floating point
    OP_MUL_F                = 34,    // Multiply floating point
    OP_ITOF                 = 42,    // Integer to float
    OP_CMPGT_F              = 44,    // Floating point greater than
    OP_CMPLT_F              = 46,    // Floating point less than
    OP_CMPGE_F              = 45,    // Floating point greater or equal
    OP_CMPLE_F              = 47,    // Floating point less than or equal
    OP_CMPEQ_F              = 48,    // Floating point equal
    OP_CMPNE_F              = 49,    // Floating point not-equal
    OP_BREAKPOINT           = 62,
    OP_SYSCALL              = 63
} instruction;

enum logic {
    PIPE_MEM,
    PIPE_INT_ARITH,
    PIPE_FLOAT_ARITH
} pipeline_sel;


void test_mult(VerilatedVcdC*, int a, int b);
inline void dump(VerilatedVcdC*);

Vsimtop *uut;					// instantiation of module
vluint64_t main_time = 0;       // Current simulation time

double sc_time_stamp () {       // Called by $time in Verilog
    return main_time;           // converts to double, to match
    // what SystemC does
}

uint32_t getFloatInt(float fl) {
	uint32_t res;
	memcpy(&res, &fl, sizeof(float));
	return res;
}

float getIntFromFloat(uint32_t it) {
	float res;
	memcpy(&res, &it, sizeof(float));
	return res;
}


typedef struct {
	uint32_t a;
	uint32_t b;
	uint32_t instr;
	uint32_t pip_sel;
	float sol;
} Testcase;


void initData(Testcase *tc, int ops) {
	for (int i = 0; i < 20; i++) {
		tc[i].a = getFloatInt(TESTVALSA[i]);
		tc[i].b = getFloatInt(TESTVALSB[i]);
		switch (ops) {
			case 0:
				tc[i].instr = OP_MUL_F;
				tc[i].pip_sel = PIPE_FLOAT_ARITH;
				tc[i].sol = TESTVALSSOL[i];
				break;
			case 1:
				tc[i].instr = OP_ADD_F;
				tc[i].pip_sel = PIPE_FLOAT_ARITH;
				tc[i].sol = TESTVALSSOL2[i];
				break;
		}
	}
}


void checkDataMul(Testcase *tc, Vsimtop *uut, vluint64_t run) {
	if (run < 6)
		return;

	if (run > 5+20)
		return;

	float sol = getIntFromFloat(uut->of_output_1);
	if (tc[run-6].sol == sol)
		std::cout << "Mul matching. \t\t";
	else 
		std::cout << "Mul not matching.\t";

	std::cout << "Got: " \
		<< std::setw(5) << sol\
		<< "\texpected: " << tc[run-6].sol;

	if (tc[run-6].sol != sol)
		std::cout << "\t Relativer Fehler: " << std::printf("%.2f",\
			100*fabs(sol-tc[run-6].sol)/tc[run-6].sol) << "%";

	std::cout << std::endl;
	
}

void checkDataAdd(Testcase *tc, Vsimtop *uut, vluint64_t run) {
	if (run <= 5)
		return;

	if (run > 5+20)
		return;

	float sol = getIntFromFloat(uut->of_output_1);
	if (tc[run-6].sol == sol)
		std::cout << "Add matching. \t\t";
	else 
		std::cout << "Add not matching.\t";

	std::cout << "Got: " \
		<< std::setw(5) << sol\
		<< "\texpected: " << tc[run-6].sol;

	if (tc[run-6].sol != sol)
		std::cout << "\t Relativer Fehler: " << std::printf("%.2f",\
			100*fabs(sol-tc[run-6].sol)/tc[run-6].sol) << "%";

	std::cout << std::endl;
	
}

int main(int argc, char** argv)
{
	Testcase tcs[20];

    // turn on trace or not?
    bool vcdTrace = true;
    VerilatedVcdC* tfp = NULL;

    Verilated::commandArgs(argc, argv);   // Remember args
    uut = new Vsimtop;   // Create instance

    uut->eval();
    uut->eval();

    if (vcdTrace)
    {
        Verilated::traceEverOn(true);

        tfp = new VerilatedVcdC;
        uut->trace(tfp, 99);

        std::string vcdname = argv[0];
        vcdname += ".vcd";
        std::cout << vcdname << std::endl;
        tfp->open(vcdname.c_str());
    }

    uut->clk = 0;
    uut->eval();

	long run = 0;
	long lrun = 0;

    while (!Verilated::gotFinish())
    {
        uut->clk = uut->clk ? 0 : 1;       // Toggle clock
        uut->eval();            // Evaluate model

		dump(tfp);

		if (run == 0) {
			initData(tcs, lrun);
		}

 
        main_time++;            // Time passes...

		if (!uut->clk) {
			uut->of_operand1_1 = tcs[run].a;
			uut->of_operand2_1 = tcs[run].b;
			uut->of_pipeline_sel  = tcs[run].pip_sel;
			uut->of_instruction_i = tcs[run].instr;;

			run++;
		} else {
			switch (lrun) {
				case 0:
					checkDataMul(tcs, uut, run);
					break;
				case 1:
					checkDataAdd(tcs, uut, run);
					break;
			}
		}

		if (run > 25) {
			std::cout << std::endl;
			run = 0;
			lrun++;
		}


		if (lrun >= 1 && run > 21)
				break;
    }

    uut->final();               // Done simulating

    if (tfp != NULL)
    {
        tfp->close();
        delete tfp;
    }

    delete uut;

    return 0;
}


inline void dump(VerilatedVcdC* tfp) {

	if (tfp != NULL)
	{
		tfp->dump (main_time);
	}

}
