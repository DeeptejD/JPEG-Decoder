#include <iostream>
#include <fstream>
#include "decoder_functions.cxx"
// #include "jpg.h"

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

        delete header;
    }

    return 0;
}