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
    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;

    for (uint i = 0; i, mcuHeight * mcuWidth; i++)
        for (uint j = 0; j < header->numComponents; j++)
            dequantizeMCUComponent(header->quantizationTables[header->colorComponents[j].quantizationTableID], mcus[i][j]);
}