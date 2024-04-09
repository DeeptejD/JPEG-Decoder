#include "jpg.h"
#include <bits/stdc++.h>

using namespace std;

void readAPPN(ifstream &inFile, Header *const header)
{
    // const just makes sure the header does not point to anything else, we can still make changes to its contents
    cout << "Reading APPN markers\n";
    uint length = (inFile.get() << 8) + inFile.get(); // this is in big endian

    for (int i = 0; i < length - 2; i++) // -2 cuz we read the first 2
    {
        inFile.get(); // we need to advance our position in the file
    }
}

Header *readJPG(const string &filename) // takes in the filename as a constant reference
{
    // open file
    ifstream inFile = ifstream(filename, ios::in | ios::binary); // opening the file in binary
    if (!inFile.is_open())
    {
        cout << "Error: error opening file\n";
        return nullptr;
    }

    // nothrow makes sure that these dont throw an exception on failure but simply return a nullptr
    Header *header = new (nothrow) Header;
    if (header == nullptr)
    {
        cout << "Error: Memory error\n";
        inFile.close();
        return nullptr;
    }

    // now we start reading the file
    // since markers are 2 bytes long we read 2 bytes at a time
    byte last = inFile.get();
    byte current = inFile.get();

    if (last != 0xFF || current != SOI)
    {
        header->valid = false;
        inFile.close();
        return header;
    }

    last = inFile.get(), current = inFile.get();

    // we have verfied that the first 2 markers exist
    while (header->valid)
    {
        if (!inFile)
        {
            cout << "Error: File ended prematurely\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        if (last != 0xFF)
        {
            cout << "Error: Expected a marker\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        if (current >= APP0 && current <= APP15)
        {
            readAPPN(inFile, header); // function whole sole resp is to read a file with that marker and store it in the header
            break;
        }

        last = inFile.get(), current = inFile.get();
    }
    return header;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        cout << "Error: Invalid arguments\n";
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        const string filename(argv[i]);
        Header *header = readJPG(filename);
        if (header == nullptr)
            continue;
        else if (!header->valid)
        {
            cout << "Error: Invalid JPG\n";
            delete header;
            continue;
        }

        // Decode the huffman data
    }
    return 0;
}
