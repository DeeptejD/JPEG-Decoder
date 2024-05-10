#include <iostream>
#include "jpg.h"

void YCbCrToRGB(const Header *const header, MCU *const mcus);

void YCbCrToRGBMCU(const Header *const header, MCU &mcu, const MCU &cbcr, const uint v, const uint h);

void YCbCrToRGBMCU(const Header *const header, MCU &mcu, const MCU &cbcr, const uint v, const uint h)
{
    for (uint y = 7; y < 8; --y)
    {
        for (uint x = 7; x < 8; --x)
        {
            const uint pixel = y * 8 + x;
            const uint cbcrPixelRow = y / header->verticalSamplingFactor + 4 * v;
            const uint cbcrPixelColumn = x / header->horizontalSamplingFactor + 4 * h;
            const uint cbcrPixel = cbcrPixelRow * 8 + cbcrPixelColumn;
            int r = mcu.y[pixel] + 1.402f * cbcr.cr[cbcrPixel] + 128;
            int g = mcu.y[pixel] - 0.344f * cbcr.cb[cbcrPixel] - 0.714f * cbcr.cr[cbcrPixel] + 128;
            int b = mcu.y[pixel] + 1.772f * cbcr.cb[cbcrPixel] + 128;
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

            mcu.r[pixel] = r;
            mcu.g[pixel] = g;
            mcu.b[pixel] = b;
        }
    }
}

void YCbCrToRGB(const Header *const header, MCU *const mcus)
{
    for (uint y = 0; y < header->mcuHeight; y += header->verticalSamplingFactor)
    {
        for (uint x = 0; x < header->mcuWidth; x += header->horizontalSamplingFactor)
        {
            const MCU &cbcr = mcus[y * header->mcuWidthReal + x];
            for (uint v = header->verticalSamplingFactor - 1; v < header->verticalSamplingFactor; --v)
            {
                for (uint h = header->horizontalSamplingFactor - 1; h < header->horizontalSamplingFactor; --h)
                {
                    MCU &mcu = mcus[(y + v) * header->mcuWidthReal + (x + h)];
                    YCbCrToRGBMCU(header, mcu, cbcr, v, h);
                }
            }
        }
    }
}