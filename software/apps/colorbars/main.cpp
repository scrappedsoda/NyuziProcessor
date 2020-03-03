//
// Copyright 2011-2015 Jeff Bush
// Copyright 2018 Sergio Schuler
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

#include <nyuzi.h>
#include <schedule.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <vga.h>
#include "Barrier.h"
#include "cb.h"
#include "Matrix2x2.h"

const int kNumThreads = 4;
const int kVectorLanes = 16;
veci16_t* frameBuffer = (veci16_t*) 0x200000;
const vecf16_t kXOffsets = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
                             8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f
                           };
const int kBytesPerPixel = 4;
const int kScreenWidth = 640;
const int kScreenHeight = 480;

Barrier<4> gFrameBarrier;
Matrix2x2 displayMatrix;

int main()
{
    int frameNum = 0;
    clock_t lastTime = 0;

    int myThreadId = get_current_thread_id();
    if (myThreadId == 0)
    {
        frameBuffer = (veci16_t*) init_vga(VGA_MODE_640x480);
        displayMatrix = Matrix2x2();
    }

    start_all_threads();

    // no rotation
    Matrix2x2 stepMatrix(
                         1.00, -0.00,
                         0.00,  1.00);

    
    // no scaling 
    stepMatrix = stepMatrix * Matrix2x2(1.00, 0.00, 0.00, 1.00);

    // Threads work on interleaved chunks of pixels.  The thread ID determines
    // the starting point.

    // 640 pixels width, 4 threads = 160 pixels per thread
    // because of SIMD, 16 pixels at a time which implies 10 strips of work per thread

    int tid = 0;
    int toggle = 0;

    while (true)
    {
        int step = 0;
        unsigned int imageBase;

        if (toggle == 0)
        {
            imageBase = (unsigned int) kImage;
        }
        else
        {
            imageBase = (unsigned int) kImage2;
        }
      
        veci16_t *outputPtr = frameBuffer + myThreadId;
        for (int y = 0; y < kScreenHeight; y++)
        {
            for (int x = myThreadId * kVectorLanes; x < kScreenWidth; x += kNumThreads * kVectorLanes)
            {
                vecf16_t xv = kXOffsets + float(x) - (kScreenWidth / 2);
                vecf16_t yv = float(y) - (kScreenHeight / 2);;
                vecf16_t u = xv * displayMatrix.a + yv * displayMatrix.b;
                vecf16_t v = xv * displayMatrix.c + yv * displayMatrix.d;

                veci16_t tx = (__builtin_convertvector(u, veci16_t) % (kImageWidth));
                veci16_t ty = (__builtin_convertvector(v, veci16_t) % (kImageHeight));

                for (int vdx = 0; vdx < 16; vdx++)
                {
                    if (tx[vdx] < 0) tx[vdx]+=kImageWidth;
                    if (ty[vdx] < 0) ty[vdx]+=kImageHeight;
                }

                // slowdown one thread by doing printf so we can see it drawing
	        if (myThreadId == tid && toggle == 0)
	        {
                    printf("x=%d,y=%d\n", x, y);
                }

	        veci16_t pixelPtrs = (ty * (kImageWidth * kBytesPerPixel))
		                     + (tx * kBytesPerPixel) + imageBase;
		*outputPtr = __builtin_nyuzi_gather_loadi(pixelPtrs);
		__asm("dflush %0" : : "r" (outputPtr));
		outputPtr += kNumThreads;
		
		step++;
	    }
	}

        if (myThreadId == 0)
        {
            displayMatrix = displayMatrix * stepMatrix;

            if ((frameNum++ & 31) == 0)
            {
                unsigned int currentTime = clock();
                if (lastTime != 0)
                {
                    float deltaTime = (float)(currentTime - lastTime) / CLOCKS_PER_SEC;
                    printf("%g fps\n", (float) 32 / deltaTime);
                }

                lastTime = currentTime;
            }
        }


        gFrameBarrier.wait();
        if (toggle == 0) tid = (tid + 1) % 4;
        toggle = !toggle;
    }

    return 0;
}


