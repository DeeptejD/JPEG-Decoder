#include <iostream>
#include "jpg.h"

void YCbCrToRGB(const Header *const header, MCU *const mcus);

void YCbCrToRGBMCU(MCU &mcu);

void YCbCrToRGBMCU(MCU &mcu)
{
    for (uint i = 0; i < 64; ++i)
    {
        int r = mcu.y[i] + 1.402f * mcu.cr[i] + 128;
        int g = mcu.y[i] - 0.344f * mcu.cb[i] - 0.714f * mcu.cr[i] + 128;
        int b = mcu.y[i] + 1.772f * mcu.cb[i] + 128;
        if (r < 0)
            r = 0;
        if (r > 255)
            r = 255;
        if (g < 0)
            g = 0;
        if (g > 255)
            g = 255;
        if (b < 0)
            b = 0;
        if (b > 255)
            b = 255;

        mcu.r[i] = r;
        mcu.g[i] = g;
        mcu.b[i] = b;
    }
}

void YCbCrToRGB(const Header *const header, MCU *const mcus)
{
    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;

    for (uint i = 0; i < mcuHeight * mcuWidth; i++)
    {
        YCbCrToRGBMCU(mcus[i]);
    }
}