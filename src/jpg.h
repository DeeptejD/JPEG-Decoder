#ifndef JPG_H
#define JPG_H
#include <vector>
#include <math.h>

// this is just renaming stuff
typedef unsigned char byte;
typedef unsigned int uint;

// Start of Frame markers, non-differential, Huffman coding
const byte SOF0 = 0xC0; // Baseline DCT
const byte SOF1 = 0xC1; // Extended sequential DCT
const byte SOF2 = 0xC2; // Progressive DCT
const byte SOF3 = 0xC3; // Lossless (sequential)

// Start of Frame markers, differential, Huffman coding
const byte SOF5 = 0xC5; // Differential sequential DCT
const byte SOF6 = 0xC6; // Differential progressive DCT
const byte SOF7 = 0xC7; // Differential lossless (sequential)

// Start of Frame markers, non-differential, arithmetic coding
const byte SOF9 = 0xC9;  // Extended sequential DCT
const byte SOF10 = 0xCA; // Progressive DCT
const byte SOF11 = 0xCB; // Lossless (sequential)

// Start of Frame markers, differential, arithmetic coding
const byte SOF13 = 0xCD; // Differential sequential DCT
const byte SOF14 = 0xCE; // Differential progressive DCT
const byte SOF15 = 0xCF; // Differential lossless (sequential)

// Define Huffman Table(s)
const byte DHT = 0xC4;

// JPEG extensions
const byte JPG = 0xC8;

// Define Arithmetic Coding Conditioning(s)
const byte DAC = 0xCC;

// Restart interval Markers
const byte RST0 = 0xD0;
const byte RST1 = 0xD1;
const byte RST2 = 0xD2;
const byte RST3 = 0xD3;
const byte RST4 = 0xD4;
const byte RST5 = 0xD5;
const byte RST6 = 0xD6;
const byte RST7 = 0xD7;

// Other Markers
const byte SOI = 0xD8; // Start of Image
const byte EOI = 0xD9; // End of Image
const byte SOS = 0xDA; // Start of Scan
const byte DQT = 0xDB; // Define Quantization Table(s)
const byte DNL = 0xDC; // Define Number of Lines
const byte DRI = 0xDD; // Define Restart Interval
const byte DHP = 0xDE; // Define Hierarchical Progression
const byte EXP = 0xDF; // Expand Reference Component(s)

// APPN Markers (place to put application specific data, like PS puts its info)
const byte APP0 = 0xE0; // JFIF marker, limited version of JPEG format
const byte APP1 = 0xE1;
const byte APP2 = 0xE2;
const byte APP3 = 0xE3;
const byte APP4 = 0xE4;
const byte APP5 = 0xE5;
const byte APP6 = 0xE6;
const byte APP7 = 0xE7;
const byte APP8 = 0xE8;
const byte APP9 = 0xE9;
const byte APP10 = 0xEA;
const byte APP11 = 0xEB;
const byte APP12 = 0xEC;
const byte APP13 = 0xED;
const byte APP14 = 0xEE;
const byte APP15 = 0xEF;

// Misc Markers
const byte JPG0 = 0xF0;
const byte JPG1 = 0xF1;
const byte JPG2 = 0xF2;
const byte JPG3 = 0xF3;
const byte JPG4 = 0xF4;
const byte JPG5 = 0xF5;
const byte JPG6 = 0xF6;
const byte JPG7 = 0xF7;
const byte JPG8 = 0xF8;
const byte JPG9 = 0xF9;
const byte JPG10 = 0xFA;
const byte JPG11 = 0xFB;
const byte JPG12 = 0xFC;
const byte JPG13 = 0xFD;
const byte COM = 0xFE;
const byte TEM = 0x01;

struct QuantizationTable
{
    // made a 1D array as single iterator can be used as well as 2 indices
    // use nested:
    // for(i: 1 -> 8)
    //     for(j: 1 -> 8)
    //          table[y * 8 + x] to access table[x][y]

    uint table[64] = {0}; // this is a 1D array instead of 2D because its more simpler
    bool set = false;     // whenw we populate a quant table we set this to true
};

struct ColorComponent
{

    byte horizontalSamplingFactor = 1;
    byte verticalSamplingFactor = 1;
    byte quantizationTableID = 0;

    byte HuffmanDCTableID = 0;
    byte HuffmanACTableID = 0;

    bool used = false; // keeps a check whether this color component is used in the img or not
};

struct HuffmanTable
{
    byte offset[17] = {0}; // there are 16(len 1 to 16) groups, the next grp offset suggests the ending of the current and so we have one extra so that the last one can also have an ending
    byte symbols[162] = {0};
    uint codes[162] = {0}; // same as the size of the symbols array (but init with uint because codes can be longer than 8bits)
    bool set = false;
};

struct Header
{
    QuantizationTable quantizationTables[4]; // we will mostly use the first 2 (1 for lum and 1 for croma)

    // id can be between 0->3 => 4 HT of each type are possible
    HuffmanTable huffmanDCTables[4];
    HuffmanTable huffmanACTables[4];

    // this flag indicates if the file is valid or not

    byte frameType = 0; // 0 is baseline (which we support)
    uint height = 0;
    uint width = 0;
    byte numComponents = 0; // 1(grayscale) or 3(rgb)
    bool zeroBased = false; // This is to support JPEG's that have their Component ID's starting from zero instead of 1. (gorilla.jpg)

    byte startOfSelection = 0;
    byte endOfSelection = 63;
    byte successiveApproximationHigh = 0;
    byte successiveApproximationLow = 0;

    uint restartInterval = 0; // this means never restart (or reset the value of DC coefficent to zero) Context: DRI Marker

    ColorComponent colorComponents[3];

    // stores the huffman data
    std::vector<byte> huffmanData;

    bool valid = true; // set to false when we encounter something illegal in the file

    uint mcuHeight = 0;
    uint mcuWidth = 0;
    uint mcuHeightReal = 0;
    uint mcuWidthReal = 0;

    byte horizontalSamplingFactor = 1;
    byte verticalSamplingFactor = 1;
};

struct MCU
{
    // why use union? (to give same addr. in memory different names)
    // sometimes a mcu might be representing rgb values instead of ycbcr
    // thus union so that the same array can be called by 2 different names
    union
    {
        int y[64] = {0};
        int r[64];
    };
    union
    {
        int cb[64] = {0};
        int g[64];
    };
    union
    {
        int cr[64] = {0};
        int b[64];
    };

    // we defined this since we wanted to access indiv components of the MCU in huffman_functions.cxx/decodeHuffmanTable function
    int *operator[](uint i)
    {
        switch (i)
        {
        case 0:
            return y;
        case 1:
            return cb;
        case 2:
            return cr;
        default:
            return nullptr;
        }
    }
};

// IDCT scaling factors (S-Factors)
const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * M_PI);
const float ml = 2.0 * std::cos(2.0 / 16.0 * 2.0 * M_PI);
const float m3 = 2.0 * std::cos(2.0 / 16.8 * 2.0 * M_PI);
const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * M_PI);
const float m2 = m0 - m5;
const float m4 = m0 + m5;

const float s0 = std::cos(0.0 / 16.0 * M_PI) / std ::sqrt(8);
const float s1 = std::cos(1.0 / 16.0 * M_PI) / 2.0;
const float s2 = std::cos(2.0 / 16.0 * M_PI) / 2.0;
const float s3 = std::cos(3.0 / 16.0 * M_PI) / 2.0;
const float s4 = std::cos(4.0 / 16.0 * M_PI) / 2.0;
const float s5 = std::cos(5.0 / 16.0 * M_PI) / 2.0;
const float s6 = std::cos(6.0 / 16.0 * M_PI) / 2.0;
const float s7 = std::cos(7.0 / 16.0 * M_PI) / 2.0;

const byte zigZagMap[] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63};

#endif
