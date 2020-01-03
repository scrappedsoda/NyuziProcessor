#include <verilated.h>          // Defines common routines
#include "Vapp_add_top.h"

#include "verilated_vcd_c.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>

#include <ctime>
#include <iomanip>

#define TESTCASES 20
#define TESTOTAL 7

inline void dump(VerilatedVcdC*);

Vapp_add_top *uut;					// instantiation of module
vluint64_t main_time = 0;       // Current simulation time

double sc_time_stamp () {       // Called by $time in Verilog
    return main_time;           // converts to double, to match
    // what SystemC does
}


typedef struct {
	long a;
	long b;
	long sol;
} Testcase;


void initData(Testcase *tc, int ops) {
	for (int i = 0; i < TESTCASES; i++) {
		// would be easier to just use ops as multiplicator
		// but this way it is possible to realize specialized 
		// functions or testcases

		switch (ops) {
			case 0:		// Low range cases 0 < x <= 10
			case 1:		// 10 < x <= 100
			case 2:		// x < 1000
			case 3:		// x < 10000
				tc[i].a = (std::rand() % (int(std::pow(10,ops+1)- std::pow(10,ops)))) + int(std::pow(10,ops));
				tc[i].b = (std::rand() % (int(std::pow(10,ops+1)- std::pow(10,ops)))) + int(std::pow(10,ops));
				break;
			case 4:
				tc[i].a = (std::rand() % (int(std::pow(2,24)- std::pow(2,20)))) + int(std::pow(2,20));
				tc[i].b = (std::rand() % (int(std::pow(2,24)- std::pow(2,20)))) + int(std::pow(2,20));
				break;

			case 5:
				tc[i].a = (std::rand() % (int(std::pow(2,28)- std::pow(2,24)))) + int(std::pow(2,24));
				tc[i].b = (std::rand() % (int(std::pow(2,28)- std::pow(2,24)))) + int(std::pow(2,24));
				break;

			case 6:
				tc[i].a = std::rand() % int(std::pow(2,31));
				tc[i].b = std::rand() % int(std::pow(2,31));
				break;

			default:	// to avoid wrong TESTOTAL
				tc[i].a = 1;
				tc[i].b = 1;
		}
		tc[i].sol = tc[i].a + tc[i].b;
	}
}


void checkOutput(Vapp_add_top *uut, Testcase tc) {
	static bool firstTime;

	if (!firstTime) {
		std::cout 
				  << "    Val A   |"\
				  << "    Val B   |"\
				  << "    Val Cpp |"\
				  << "    Val SysV|"\
				  << "    Val App |"\
				  << "RelF SV & C |"\
				  << "RelF App & C\r\n";
		firstTime = true;
	}
	long sol1 = uut->of_output_ori;
	long appr = uut->of_output_app;
	long solc = tc.sol;

	double relf1, relf2;
	relf1 = (double) labs(sol1-solc)/solc*100;
	relf2 = (double) labs(appr-solc)/solc*100;

//	std::cout << uut->a << " " << tc.a << std::endl;
//	std::cout << uut->b << " " << tc.b << std::endl;

	std::cout\
				<< std::setw(12) << uut->a << "|"\
				<< std::setw(12) << uut->b << "|"\
				<< std::setw(12) << solc << "|"\
				<< std::setw(12) << sol1 << "|"\
				<< std::setw(12) << appr << "|";

	std::cout \
		<< std::printf("%12.2lf",relf1) << "%|"\
		<< std::printf("%12.2lf",relf2) << "%" << std::endl;
}



int main(int argc, char** argv)
{
	Testcase tcs[TESTCASES];

    // turn on trace or not?
    bool vcdTrace = true;
    VerilatedVcdC* tfp = NULL;

    Verilated::commandArgs(argc, argv);   // Remember args
    uut = new Vapp_add_top;   // Create instance

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
		if (run == 0 && !uut->clk) {
			initData(tcs, lrun);
//			for (int i = 0; i < TESTCASES; i++)
//				std::cout << tcs[i].a << " " << tcs[i].b << std::endl;
//			std::cout << "Pre Print Ende." << std::endl;
		}


		dump(tfp);


		if (!uut->clk) {
			// set values when clk is low
			uut->a = tcs[run].a;
			uut->b = tcs[run].b;

		} else {
			// check when clk is high
			checkOutput(uut, tcs[run]);
			run++;
		}

        uut->clk = uut->clk ? 0 : 1;       // Toggle clock
        uut->eval();            // Evaluate model

        main_time++;            // Time passes...

		if (run >= TESTCASES && lrun >= TESTOTAL-1)
			break;
		else if (run >= TESTCASES) {
			std::cout << "\r\n";
			lrun++;
			run=0;
		}
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
