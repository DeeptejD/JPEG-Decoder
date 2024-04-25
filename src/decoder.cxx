#include <iostream>
#include <fstream>
#include "decoder_functions.cxx"
#include "inverseDCT_functions.cxx"
#include "dequantize_functions.cxx"
#include "bitmap_output.cxx"
#include "huffman_functions.cxx"
#include "color_conversion_functions.cxx"
#include "jpg.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "error: invalid arguments\n";
        return 1;
    }

    // once we make sure that a filename has been provided, we process every arg except the first one (first one is the code file)
    for (int i = 1; i < argc; i++)
    {
        const std::string filename(argv[i]); // store the filename as a string
        Header *header = readJPG(filename); // reads the filename and returns a header that is constructed from reading the file

        if (header == nullptr)
        {
            continue;
        }
        if (header->valid == false)
        {
            std::cout << "Invalid JPG\n";
            delete header;
            continue;
        }

        printHeader(header);

        // decode Huffman data
        MCU *mcus = decodeHuffmanData(header);
        if (mcus == nullptr)
        {
            delete header;
            continue;
        }

        // dequantize MCU coefficients
        dequantize(header, mcus);

        // inverse DCT on MCUs
        inverseDCT(header, mcus);

        // color conversion
        YCbCrToRGB(header, mcus);

        // write bmp file
        const std::size_t pos = filename.find_last_of('.');
        const std::string outFilename = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
        writeBMP(header, mcus, outFilename);

        delete[] mcus;
        delete header;
    }

    return 0;
}