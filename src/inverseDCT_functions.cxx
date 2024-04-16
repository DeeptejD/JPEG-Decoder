#include <iostream>
#include <cmath>
#include <fstream>
#include "jpg.h"

// perform inverse DCT on mcu array
void inverseDCT(const Header *const header, MCU *const mcus);

// inverse DCT on each mcu
void inverseDCTComponent(const float *const idctMap, int *const component);

void inverseDCTComponent(const float *const idctMap, int *const component)
{
    int result[64] = {0};
    for (uint y = 0; y < 8; y++)
    {
        for (uint x = 0; x < 8; x++)
        {
            float sum = 0.0;
            // i was changed to v and j was changed to u to stick with the convention
            for (uint v = 0; v < 8; v++)
            {
                for (uint u = 0; u < 8; u++)
                {
                    sum += component[v * 8 + u] *
                           idctMap[u * 8 + x] *
                           idctMap[v * 8 + y];
                }
            }
            result[y * 8 + x] = (int)sum;
        }
    }

    for (uint i = 0; i < 64; i++)
        component[i] = result[i];
}

void inverseDCT(const Header *const header, MCU *const mcus)
{
    // prepare idctMap
    float idctMap[64];
    for (uint u = 0; u < 8; u++)
    {
        const float c = (u == 0) ? (1.0 / std::sqrt(2.0) / 2.0) : (1.0 / 2.0); // division by 2 is added to avoid the division of 4 we were doing with the sum in the previous function
        for (uint x = 0; x < 8; x++)
        {
            idctMap[u * 8 + x] = c * std::cos((2.0 * x + 1.0) * u * M_PI / 16.0);
        }
    }

    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;

    for (uint i = 0; i < mcuHeight * mcuWidth; i++)
        for (uint j = 0; j < header->numComponents; j++)
            inverseDCTComponent(idctMap, mcus[i][j]);
}