#include <iostream>
#include <cmath>
#include <fstream>
#include "jpg.h"

// perform inverse DCT on mcu array
void inverseDCT(const Header *const header, MCU *const mcus);

// inverse DCT on each mcu
void inverseDCTComponent(int *const component);

void inverseDCTComponent(int *const component)
{
    for (uint i = 0; i < 8; i++)
    {
        const float g0 = component[0 * 8 + i] * s0;
        const float g1 = component[4 * 8 + i] * s4;
        const float g2 = component[2 * 8 + i] * s2;
        const float g3 = component[6 * 8 + i] * s6;
        const float g4 = component[5 * 8 + i] * s5;
        const float g5 = component[1 * 8 + i] * s1;
        const float g6 = component[7 * 8 + i] * s7;
        const float g7 = component[3 * 8 + i] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * ml;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        component[0 * 8 + i] = b0 + b7;
        component[1 * 8 + i] = b1 + b6;
        component[2 * 8 + i] = b2 + b5;
        component[3 * 8 + i] = b3 + b4;
        component[4 * 8 + i] = b3 - b4;
        component[5 * 8 + i] = b2 - b5;
        component[6 * 8 + i] = b1 - b6;
        component[7 * 8 + i] = b0 - b7;
    }

    for (uint i = 0; i < 8; i++)
    {
        const float g0 = component[i * 8 + 0] * s0;
        const float g1 = component[i * 8 + 4] * s4;
        const float g2 = component[i * 8 + 2] * s2;
        const float g3 = component[i * 8 + 6] * s6;
        const float g4 = component[i * 8 + 5] * s5;
        const float g5 = component[i * 8 + 1] * s1;
        const float g6 = component[i * 8 + 7] * s7;
        const float g7 = component[i * 8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * ml;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        component[i * 8 + 0] = b0 + b7;
        component[i * 8 + 1] = b1 + b6;
        component[i * 8 + 2] = b2 + b5;
        component[i * 8 + 3] = b3 + b4;
        component[i * 8 + 4] = b3 - b4;
        component[i * 8 + 5] = b2 - b5;
        component[i * 8 + 6] = b1 - b6;
        component[i * 8 + 7] = b0 - b7;
    }
}

void inverseDCT(const Header *const header, MCU *const mcus)
{
    for (uint y = 0; y < header->mcuHeight; y += header->verticalSamplingFactor)
    {
        for (uint x = 0; x < header->mcuWidth; x += header->horizontalSamplingFactor)
        {
            for (uint i = 0; i < header->numComponents; ++i)
            {
                for (uint v = 0; v < header->colorComponents[i].verticalSamplingFactor; ++v)
                {
                    for (uint h = 0; h < header->colorComponents[i].horizontalSamplingFactor; ++h)
                    {
                        inverseDCTComponent(mcus[(y + v) * header->mcuWidthReal + (x + h)][i]);
                    }
                }
            }
        }
    }
}