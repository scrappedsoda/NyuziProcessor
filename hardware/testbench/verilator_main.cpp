//
// Copyright 2011-2015 Jeff Bush
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <iostream>
#include <cstdlib>
#include "Vsoc_tb.h"
#include "verilated.h"
#include "verilated_vpi.h"
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif

#if SDL
#include <SDL2/SDL.h>
//#include <opencv/opencv2.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#define SCREEN_WIDTH 750
#define SCREEN_HEIGHT 600
typedef struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} rgb;

cv::Mat img(SCREEN_HEIGHT, SCREEN_WIDTH, CV_8UC3);

unsigned int frame_ready;
rgb framebuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screen_surface = NULL;
SDL_Texture* texture = NULL;

void initSDL()
{

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		exit(-1);
	} 
	else 
	{
		// Create window
		window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,\
				SCREEN_WIDTH,\
				SCREEN_HEIGHT,\
				SDL_WINDOW_SHOWN );
		if (window == NULL) 
		{
			printf("Window could not be created.\n");
			exit(-2);
		} 
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,\
				SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
	}
}

void drawSDL()
{
	SDL_SetRenderDrawColor(renderer, 0,0,0,255);
	SDL_RenderClear(renderer);
	// use frambeuffer
	for (int j = 0; j < img.rows; ++j) {
		for (int i = 0; i < img.cols; ++i) {
			cv::Vec3b tmp = img.at<cv::Vec3b>(img.rows-j-1,i);
			SDL_SetRenderDrawColor(renderer, tmp[2], tmp[1], tmp[0], 255);
			SDL_RenderDrawPoint(renderer, i, img.rows-j-1);
		}
	}
	SDL_RenderPresent(renderer);
}

void killSDL()
{
	SDL_DestroyWindow(window);
	SDL_Quit();

}

void clearSDL()
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	for (int j = 0; j < SCREEN_HEIGHT; ++j) {
		for (int i = 0; i < SCREEN_WIDTH; ++i) {
			SDL_RenderDrawPoint(renderer, i, SCREEN_HEIGHT-j-1);
		}
	}
}
#endif
// SDL Stuff ende



//
// This is compiled into the verilog simulator executable along with the
// source files generated by Verilator. It initializes and runs the simulation
// loop for the full processor.
//

namespace
{
vluint64_t currentTime = 0;
}

// Called whenever the $time variable is accessed.
double sc_time_stamp()
{
    return currentTime;
}

int main(int argc, char **argv, char **env)
{
    Verilated::commandArgs(argc, argv);
    Verilated::debug(0);

    Vsoc_tb* testbench = new Vsoc_tb;


#if SDL
	std::cout << "Start with SDL Rendering of VGA output." << std::endl;
	bool vga_last_clk = 0;
	int pixel_count= 0;
	int hsync_pulses=0;
	bool vga_last_hsync;
	bool vga_last_vsync;
	int x=0, y=0;
	initSDL();
#endif
    // As with real hardware, reset is a bit tricky.
    // - Most assertions will fail before the design has been reset.
    // - Assertions are not tested while reset is asserted.
    // BUT:
    // - Many blocks require a positive edge on reset to trigger
    //   (not all, any block that also triggers on clock will synchronously
    //   reset if it is asserted).
    //
    // This is a bit of a hack, set the 'last' state of reset to zero and reset to one.
    // This will cause a positive edge event on the next eval() that will trigger
    // all reset blocks. Reset will be deasserted in the main loop below.
    testbench->__Vclklast__TOP__reset = 0;
    testbench->reset = 1;
    testbench->clk = 0;
    testbench->eval();

#if VM_TRACE // If verilator was invoked with --trace
    Verilated::traceEverOn(true);
    VL_PRINTF("Writing waveform to trace.vcd\n");
    VerilatedVcdC* tfp = new VerilatedVcdC;
    testbench->trace(tfp, 99);
    tfp->open("trace.vcd");
#endif

    while (!Verilated::gotFinish())
    {
        // Run for a few clock cycles with reset asserted. This allows flops
        // that are not reset to settle on valid values so assertions don't
        // trip.
        if (currentTime == 4)
            testbench->reset = 0;

        testbench->clk = !testbench->clk;
        testbench->eval();
#if VM_TRACE
        tfp->dump(currentTime); // Create waveform trace for this timestamp
#endif

#if SDL
		if (testbench->vga_vs_o == 1) {
			if (testbench->vga_hs_o == 1) {
				if (testbench->vga_clk_o == 1 && vga_last_clk == 0) {
					cv::Vec3b tmp;
					 
					tmp[2] = testbench->vga_r_o;
					tmp[1] = testbench->vga_g_o;
					tmp[0] = testbench->vga_b_o;

					assert(x >= 0);
					assert(y >= 0);
					assert(x < img.cols);
					assert(y < img.rows);

					img.at<cv::Vec3b>(y,x) = tmp;
					// framebuffer[y][x] = tmp;
					++x;
					++pixel_count;

//					std::cout << "rgb: " 
//						<< testbench->vga_r_o 
//						<< testbench->vga_g_o 
//						<< testbench->vga_b_o 
//						<< "\n";
				}
			}

			if (testbench->vga_hs_o == 0 && testbench->vga_hs_o != vga_last_hsync)
			{
//				std::cout << "Pixel Count: " << pixel_count <<"\n";
				drawSDL();
				pixel_count = 0;
				x=0;
//				drawSDL();
			}
			if (testbench->vga_hs_o == 1&& vga_last_hsync == 0) {
				++y;
				++hsync_pulses;
			}
//			if (testbench->vga_hs_o == 0 && vga_last_hsync == 1)
//				std::cout << "Zeile fertig\n";
			
		}
		if (testbench->vga_vs_o == 0 && vga_last_vsync == 1) {
			drawSDL();
			y=0;
		}
		vga_last_vsync = testbench->vga_vs_o;
		vga_last_hsync = testbench->vga_hs_o;
		vga_last_clk = testbench->vga_clk_o;
#endif

        currentTime++;
    }
	
	std::cout << "Simulation Ende...";
	getchar();
    testbench->final();
#if SDL
	cv::imwrite("output.png", img);
	killSDL();
#endif

#if VM_TRACE
    tfp->close();
#endif

    delete testbench;

    return 0;
}
