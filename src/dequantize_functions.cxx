#include <iostream>
#include <fstream>
#include "jpg.h"

// performs dequantization on each mcu
void dequantize(const Header *const header, MCU *const mcus);

// dequantizes an mcu (multiplies with respective value)
void dequantizeMCUComponent(const QuantizationTable &qTable, int *const component);

void dequantizeMCUComponent(const QuantizationTable &qTable, int *const component)
{
    for (uint i = 0; i < 64; i++)
        component[i] *= qTable.table[i];
}

void dequantize(const Header *const header, MCU *const mcus)
{
    for (uint y = 0; y < header->mcuHeight; y += header->verticalSamplingFactor)
        for (uint x = 0; x < header->mcuWidth; x += header->horizontalSamplingFactor)
        {
            for (uint i = 0; i < header->numComponents; ++i)
            {
                for (uint v = 0; v < header->colorComponents[i].verticalSamplingFactor; ++v)
                {
                    for (uint h = 0; h < header->colorComponents[i].horizontalSamplingFactor; ++h)
                    {
                        dequantizeMCUComponent(header->quantizationTables[header->colorComponents[i].quantizationTableID], mcus[(y + v) * header->mcuWidthReal + (x + h)][i]);
                    }
                }
            }
        }
}