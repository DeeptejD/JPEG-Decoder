#include <iostream>
#include <fstream>
#include "decoder_functions.cxx"
#include "dequantize_functions.cxx"
#include "bitmap_output.cxx"
#include "huffman_functions.cxx"
#include "jpg.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "error: invalid arguments\n";
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        const std::string filename(argv[i]);
        Header *header = readJPG(filename);

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


        // write bmp file
        const std::size_t pos = filename.find_last_of('.');
        const std::string outFilename = (pos == std::string::npos) ? (filename + ".bmp") : (filename.substr(0, pos) + ".bmp");
        writeBMP(header, mcus, outFilename);

        delete[] mcus;
        delete header;
    }

    return 0;
}