#include <iostream>
#include "jpg.h"

class BitReader;

byte getNextSymbol(BitReader &b, const HuffmanTable &hTable);

bool decodeMCUComponent(BitReader &b, int *const component, int &previousDC, const HuffmanTable &dcTable, const HuffmanTable &acTable);

// decode all the Huffman data and fill all MCUs
MCU *decodeHuffmanData(Header *const header);

// generates all the huffman codes from their frequencies
void generateCodes(HuffmanTable &hTable);

// Definitions

void generateCodes(HuffmanTable &hTable)
{
    uint code = 0; // 1 bit long code
    for (uint i = 0; i < 15; i++)
    {
        for (uint j = hTable.offset[i]; j < hTable.offset[i + 1]; j++)
        {
            hTable.codes[j] = code;
            code += 1; // increment by 1
        }
        code <<= 1; // left shift by 1, so a zero is appended to the right
    }
}

// helper class to read bits from a byte vector
class BitReader
{
private:
    uint nextByte = 0;
    uint nextBit = 0;
    const std::vector<byte> &data;

public:
    BitReader(const std::vector<byte> &d) : data(d) // using initalizer list to init d since we cant init a const in the body of a constructor
    {
    }

    // read one bit (0 or 1) or return -1 if all bits have already been read
    int readBit()
    {
        if (nextByte >= data.size())
            return -1;
        int bit = (data[nextByte] >> (7 - nextBit)) & 1; // we do 7 - to maintain a order of MSB to LSB of the read bits
        nextBit += 1;
        if (nextBit == 0)
        {
            nextBit = 0;
            nextByte += 1;
        }
        return bit;
    }

    // read a varaible number of bits
    // first read bit is MSB
    // return -1 if at any point all bits have already been read
    int readBits(const uint length)
    {
        int bits = 0;
        for (uint i = 0; i < length; i++)
        {
            int bit = readBit(); // to get the next bit out of the bitstream
            if (bit == -1)
            {
                bits = -1;
                break;
            }
            bits = (bits << 1) | bit; // add the read bit to the bitstream by ORing
        }
        return bits;
    }

    void align()
    {
        if (nextByte >= data.size())
        {
            return;
        }
        if (nextBit != 0)
        {
            nextBit = 0;
            nextByte += 1;
        }
    }
};

byte getNextSymbol(BitReader &b, const HuffmanTable &hTable)
{
    uint currentCode = 0;
    for (uint i = 0; i < 16; i++)
    {
        int bit = b.readBit();
        if (bit == -1)
            return -1; // since the return type is byte, which is char (0-255), when we return -1 it gets forced into the valid range and returns 255 instead
        currentCode = (currentCode << 1) | bit;
        for (uint j = hTable.offset[i]; j < hTable.offset[i + 1]; j++)
        {
            // j's are all the indices of all the codes of same length
            return hTable.symbols[j];
        }
    }
    return -1; // after reading 16 bits we never found a match
}

bool decodeMCUComponent(BitReader &b, int *const component, int &previousDC, const HuffmanTable &dcTable, const HuffmanTable &acTable)
{
    // uses the dc and ac tables to extract the dc and ac coeffs

    // get the DC value for this MCU component
    byte length = getNextSymbol(b, dcTable);
    if (length == (byte)-1)
    {
        std::cout << "Error: Invalid DC value\n";
        return false;
    }
    if (length > 11) // we know that DC coeff shud never have a length > 11
    {
        std::cout << "Error: DC coefficient length greater than 11\n";
        return false;
    }

    int coeff = b.readBits(length);
    if (coeff == -1)
    {
        std::cout << "Error: Invalid DC value\n";
        return false;
    }

    // now we check if this DC coeff needs to be translated to a negative val or not
    if (length != 0 && coeff < (1 << (length - 1))) // only DC coefficients are allowed to have a length of zero, so we need to also keep a check to make sure length is not zero
    {
        coeff -= (1 << length) - 1;
    }

    // this part makes sure that DC coeffs are relative to the DC coeff of the prev MCU
    component[0] = coeff + previousDC;
    previousDC = component[0];

    // get the AC values for this MCU component
    uint i = 1;
    while (i < 64)
    {
        byte symbol = getNextSymbol(b, acTable);
        if (symbol == (byte)-1)
        {
            std::cout << "Error: Invalid AC value\n";
            return false;
        }

        // symbol 0x00 means fill remainder of componentnwith 0
        if (symbol == 0x00)
        {
            for (; i < 64; i++)
            {
                component[zigZagMap[i]] = 0; // we dont go in a straight line thats why ziggy zaggy
            }
            return true;
        }

        // otherwise, read next component coefficient
        byte numZeroes = symbol >> 4;     // upper nibble
        byte coeffLength = symbol & 0X0F; // lower nibble
        coeff = 0;

        // symbol 0x0F means skip 16 0's
        if (symbol == 0xF0)
        {
            numZeroes = 16;
        }

        // smol loop •⩊•
        if (i + numZeroes >= 64)
        {
            std::cout << "Error >.<!: Zero run-length exceeded MCU\n";
            return false;
        }
        for (uint j = 0; j < numZeroes; i++, j++)
        {
            component[zigZagMap[i]] = 0;
        }

        if (coeffLength > 10) // AC coeffs cant have a length greater than 10
        {
            std::cout << "Error: AC coefficient length greater than 10\n";
            return false;
        }
        if (coeffLength != 0)
        {
            coeff = b.readBits(coeffLength);
            if (coeff == -1) // error with the bitreader
            {
                std::cout << "Error: Invalid AC value\n";
                return false;
            }

            if (coeff < (1 << (coeffLength - 1)))
            {
                coeff -= (1 << coeffLength) - 1;
            }

            component[zigZagMap[i]] = coeff;
            i += 1;
        }
    }
}

MCU *decodeHuffmanData(Header *const header)
{
    const uint mcuHeight = (header->height + 7) / 8;
    const uint mcuWidth = (header->width + 7) / 8;
    MCU *mcus = new (std::nothrow) MCU[ mcuHeight * mcuWidth ];

    if (mcus == nullptr)
    {
        std::cout << "Error - Memory error\n";
    }

    for (uint i = 0; i < 4; i++)
    {
        if (header->huffmanDCTables[i].set)
        {
            generateCodes(header->huffmanDCTables[i]);
        }
        if (header->huffmanACTables[i].set)
        {
            generateCodes(header->huffmanDCTables[i]);
        }
    }

    BitReader b(header->huffmanData);

    int previousDCs[3] = {0};

    for (uint i = 0; i < mcuHeight * mcuWidth; i++) // loops for every mcu
    {
        // at the strt of an MCU
        if (header->restartInterval != 0 && i & header->restartInterval == 0) // its time to restart
        {
            // at the end of the restart interval we have to reset the previous DCs
            previousDCs[0] = 0;
            previousDCs[1] = 0;
            previousDCs[2] = 0;

            b.align();
        }
        // fill it with the coefficients from the huffman data
        for (uint j = 0; j < header->numComponents; j++) // run a function for all the component in that MCU
        {
            // we call a function whose responsibility is to process a single channel of a single MCU
            if (!decodeMCUComponent(b,
                                    mcus[i][j],
                                    previousDCs[j],
                                    header->huffmanDCTables[header->colorComponents[i].HuffmanDCTableID],
                                    header->huffmanACTables[header->colorComponents[j].HuffmanACTableID])) // we only realistically want to pass the current component
            {
                delete[] mcus;
                return nullptr;
            }
        }
    }

    return mcus;
}