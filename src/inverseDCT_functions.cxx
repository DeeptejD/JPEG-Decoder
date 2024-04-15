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
    int result[64] = {0};
    for (uint y = 0; y < 8; y++)
    {
        for (uint x = 0; x < 8; x++)
        {
            double sum = 0.0;
            for (uint i = 0; i < 8; i++)
            {
                for (uint j = 0; j < 8; j++)
                {
                    double ci = 1.0;
                    double cj = 1.0;
                    if (i == 0)
                        ci = 1.0 / std::sqrt(2.0);
                    if (j == 0)
                        cj = 1.0 / std::sqrt(2.0);
                    sum += ci * cj * component[i * 8 + j] *
                           std::cos((2.0 * x + 1.0) * j * M_PI / 16.0) *
                           std::cos((2.0 * y + 1.0) * i * M_PI / 16.0);
                }
            }
            sum /= 4.0;
            result[y * 8 + x] = (int)sum;
        }
    }

    for (uint i = 0; i < 64; i++)
        component[i] = result[i];
}

void inverseDCT(const Header *const header, MCU *const mcus)
{
    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;

    for (uint i = 0; i < mcuHeight * mcuWidth; i++)
        for (uint j = 0; j < header->numComponents; j++)
            inverseDCTComponent(mcus[i][j]);
}