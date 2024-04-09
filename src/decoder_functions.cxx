#include <iostream>
#include <fstream>
#include "jpg.h"

// read appn marker
void readAPPN(std::ifstream &inFile, Header *const header);

// read quantization table (DQT) marker
void readQuantizationTable(std::ifstream &inFile, Header *const header);

// reads the jpg and calls other dub-readers
Header *readJPG(const std::string &filename);

// reads start of frame
void readStartOfFrame(std::ifstream &inFile, Header *const header);

// prints the content of header
void printHeader(const Header *const header);

void readStartOfFrame(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading SOF Marker\n";
    if (header->numComponents != 0)
    {
        std::cout << "ERROR: Multiple SOFs detected\n";
        header->valid = false;
        return;
    }

    uint length = (inFile.get() << 8) + inFile.get();
    byte precision = inFile.get();

    if (precision != 8)
    {
        std::cout << "ERROR: Invalid precision: " << (uint)precision << "\n";
        header->valid = false;
        return;
    }

    header->height = (inFile.get() << 8) + inFile.get();
    header->width = (inFile.get() << 8) + inFile.get();
    if (header->height == 0 || header->width == 0)
    {
        std::cout << "ERROR: Invalid dimensions\n";
        header->valid = false;
        return;
    }

    header->numComponents = inFile.get();
    if (header->numComponents == 4)
    {
        std::cout << "ERROR: CMYK color mode not supported\n";
        header->valid = false;
        return;
    }
    if (header->numComponents == 0)
    {
        std::cout << "ERROR: Number of components musnt be zero\n";
        header->valid = false;
        return;
    }

    for (uint i = 0; i < header->numComponents; i++)
    {
        byte componentID = inFile.get();
        if (componentID == 4 || componentID == 5)
        {
            std::cout << "ERROR: YIQ color mode not supported\n";
            header->valid = false;
            return;
        }

        ColorComponent *component = &header->colorComponents[componentID - 1];
        if (component->used)
        {
            std::cout << "ERROR: Duplicate color component ID\n";
            header->valid = false;
            return;
        }
        component->used = true;

        byte samplingFactor = inFile.get();
        // Sampling factor is also split into upper and lower nibble
        component->horizontalSamplingFactor = samplingFactor >> 4;
        component->verticalSamplingFactor = samplingFactor & 0x0F;
        component->quantizationTableID = inFile.get();

        if (component->horizontalSamplingFactor != 1 || component->verticalSamplingFactor != 1)
        {
            std::cout << "ERROR: Sampling factors not supported\n";
            header->valid = false;
            return;
        }

        if (component->quantizationTableID > 3)
        {
            std::cout << "ERROR: Invalis quantization table id in frame components\n";
            header->valid = false;
            return;
        }
    }

    if (length - 8 - (3 * header->numComponents) != 0)
    {
        std::cout << "ERROR: SOF invalid\n";
        header->valid = false;
        return;
    }
}

void printHeader(const Header *const header)
{
    if (header == nullptr)
        return;

    // print the 4 quantization tables (how many exist)
    std::cout << "DQT--------------\n";
    for (int i = 0; i < 4; i++)
    {
        if (header->quantizationTables[i].set)
        {
            std::cout << "Table ID: " << i << "\n";
            std::cout << "Table Data: ";

            for (uint j = 0; j < 64; j++)
            {
                if (j % 8 == 0)
                    std::cout << "\n";

                std::cout << header->quantizationTables[i].table[j] << "\t ";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\nSOF --";
    std::cout << "Frame type: 0x" << std::hex << (uint)header->frameType << std::dec << "\n";
    std::cout << "Height: " << header->height << "\n";
    std::cout << "Width: " << header->width << "\n";
    std::cout << "Color components: \n";
    for (uint i = 0; i < header->numComponents; i++)
    {
        std::cout << "\nComponent ID: " << (i + 1) << "\n";
        std::cout << "Horizontal Sampling Factor: " << (uint)header->colorComponents[i].horizontalSamplingFactor << "\n";
        std::cout << "Vertical Sampling Factor: " << (uint)header->colorComponents[i].verticalSamplingFactor << "\n";
        std::cout << "Quantization Table ID: " << (uint)header->colorComponents[i].quantizationTableID << "\n";
    }
}

void readAPPN(std::ifstream &inFile, Header *const header)
{
    // const just makes sure the header does not point to anything else, we can still make changes to its contents
    std::cout << "Reading APPN Marker...\n";
    uint length = (inFile.get() << 8) + inFile.get(); // this is in big endian

    for (uint i = 0; i < length - 2; i++) // -2 cuz we read the first 2
        inFile.get();
}

void readQuantizationTable(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading DQT Marker...\n";
    int length = (inFile.get() << 8) + inFile.get();
    length -= 2;

    // length is int not uint coz it can go negative in case of an error
    while (length > 0)
    {
        byte tableInfo = inFile.get();
        length--;

        // table id is the lower nibble of tableInfo
        byte tableID = tableInfo & 0x0F;

        // jpeg permits max 4 quantization tables
        if (tableID > 3)
        {
            std::cout << "ERROR: Invalid quantizaiton table ID: " << (uint)tableID << "\n";
            header->valid = false;
            return;
        }

        header->quantizationTables[tableID].set = true;

        // upper nibble of tableInfo == 0 => 16b qt table
        //                           == 1 -> 8b qt table
        if (tableInfo >> 4 != 0)
        {
            for (uint i = 0; i < 64; i++)
                header->quantizationTables[tableID].table[zigZagMap[i]] = (inFile.get() << 8) + inFile.get();
            length -= 128; // 64 values * 16 bit
        }
        else
        {
            for (uint i = 0; i < 64; i++)
                header->quantizationTables[tableID].table[zigZagMap[i]] = inFile.get();
            length -= 64;
        }
    }

    if (length != 0) // negative => didnt fit in properly
    {
        std::cout << "ERROR: Invalid DQT\n";
        header->valid = false;
    }
}

Header *readJPG(const std::string &filename)
{
    // open file in binary
    std::ifstream inFile = std::ifstream(filename, std::ios::in | std::ios::binary);
    if (!inFile.is_open())
    {
        std::cout << "ERROR: Error opening input file\n";
        return nullptr;
    }

    // new wont throw error, will instead return nullptr
    Header *header = new (std::nothrow) Header;
    if (header == nullptr)
    {
        std::cout << "ERROR: Memory error\n";
        inFile.close();
        return nullptr;
    }

    // since markers are 2 bytes long we read 2 bytes at a time
    byte last = inFile.get();
    byte current = inFile.get();
    if (last != 0xFF || current != SOI)
    {
        header->valid = false;
        inFile.close();
        return header;
    }

    last = inFile.get();
    current = inFile.get();
    while (header->valid)
    {
        if (!inFile)
        {
            std::cout << "ERROR: File ended prematurely\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        if (last != 0xFF)
        {
            std::cout << "ERROR: Expected a marker\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        if (current == SOF0)
        {
            // read SOF marker
            header->frameType = SOF0;
            readStartOfFrame(inFile, header);
            break;
        }
        else if (current == DQT)
        {
            // read quantization table marker
            readQuantizationTable(inFile, header);
        }
        else if (current >= APP0 && current <= APP15)
        {
            // read appn marker
            readAPPN(inFile, header);
        }

        last = inFile.get();
        current = inFile.get();
    }

    return header;
}
