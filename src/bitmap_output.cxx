#include <iostream>
#include <fstream>
#include "jpg.h"

// declarations

// writes all pixels in the MCUs to a BMP file
void writeBMP(const Header *const header, const MCU *const mcus, const std::string &filename);

// helper function to write 4B int in little-endian
void putInt(std::ofstream &outfile, const uint v);

// helper function to write 2B short int in little-endian
void putShort(std::ofstream &outfile, const uint v);

// definitions

void writeBMP(const Header *const header, const MCU *const mcus, const std::string &filename)
{
    // open the output file
    std::ofstream outFile = std::ofstream(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open())
    {
        std::cout << "ERROR: Error opening output file\n";
        return;
    }

    // ceil(a/b) = (a + b - 1) / b
    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;
    const uint paddingSize = header->width % 4;
    const uint size = 14 + 12 + header->height * header->width * 3 + paddingSize * header->height;

    // Header first part
    outFile.put('B');
    outFile.put('M');
    putInt(outFile, size);
    putInt(outFile, 0);
    putInt(outFile, 0x1A);

    // DIB Header
    putInt(outFile, 12);
    putShort(outFile, header->width);
    putShort(outFile, header->height);
    putShort(outFile, 1);
    putShort(outFile, 24);

    // Rows into Header
    for (int y = header->height - 1; y >= 0; y--)
    {
        const uint mcuRow = y / 8;
        const uint pixelRow = y % 8;
        for (uint x = 0; x < header->width; x++)
        {
            const uint mcuColumn = x / 8;
            const uint pixelColumn = x % 8;
            const uint mcuIndex = mcuRow * mcuWidth + mcuColumn;

            const uint pixelIndex = pixelRow * 8 + pixelColumn;
            outFile.put(mcus[mcuIndex].b[pixelIndex]);
            outFile.put(mcus[mcuIndex].g[pixelIndex]);
            outFile.put(mcus[mcuIndex].r[pixelIndex]);
        }

        for (uint i = 0; i < paddingSize; i++)
        {
            outFile.put(0);
        }
    }
    outFile.close();
}

void putInt(std::ofstream &outFile, const uint v)
{
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
    outFile.put((v >> 16) & 0xFF);
    outFile.put((v >> 24) & 0xFF);
}

void putShort(std::ofstream &outFile, const uint v)
{
    outFile.put((v >> 0) & 0xFF);
    outFile.put((v >> 8) & 0xFF);
}