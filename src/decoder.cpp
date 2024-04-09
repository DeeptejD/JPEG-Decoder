#include "jpg.h"
#include <bits/stdc++.h>

using namespace std;

void readStartOfFrame(ifstream &inFile, Header *const header)
{
    cout << "Readinf SOF marker\n";
    if (header->numComponents != 0)
    {
        cout << "Error: Multiple SOFs deetcted\n";
        header->valid = false;
        return;
    }

    uint length = (inFile.get() << 8) + inFile.get();
    byte precision = inFile.get();

    if (precision != 8)
    {
        cout << "Error: Invalid precision: " << (uint)precision << endl;
        header->valid = false;
        return;
    }
    header->height = (inFile.get() << 8) + inFile.get();
    header->width = (inFile.get() << 8) + inFile.get();
    if (header->height == 0 || header->width == 0)
    {
        cout << "Error: Invalid dimensions\n";
        header->valid = false;
        return;
    }
    header->numComponents = inFile.get();
    if (header->numComponents == 4)
    {
        cout << "Error: CMYK color mode not supported\n";
        header->valid = false;
        return;
    }

    if (header->numComponents == 0)
    {
        cout << "Error: number of components cant be zero\n";
        header->valid = false;
        return;
    }

    for (uint i = 0; i < header->numComponents; i++)
    {
        byte componentID = inFile.get();
        if (componentID == 4 || componentID == 5)
        {
            // for YIQ not supported
            cout << "Error: YIQ color mode not supported\n";
            header->valid = false;
            return;
        }
        if (componentID == 0 || componentID > 3)
        {
            cout << "Error: Invalid component ID: " << (uint)componentID << endl;
            header->valid = false;
            return;
        }
        ColorComponent *component = &header->colorComponents[componentID - 1];
        if (component->used)
        {
            // same value more than once
            cout << "Error: Duplicate color component ID\n";
            header->valid = false;
            return;
        }
        component->used = true;
        byte samplingFactor = inFile.get();

        // Sampling factor is also split into upper and lower nibble
        component->horizontalSamplingFactor = samplingFactor >> 4;
        component->verticalSamplingFactor = samplingFactor & 0x0F;
        if (component->horizontalSamplingFactor != 1 || component->verticalSamplingFactor != 1)
        {
            cout << "Error: Sampling factors not supported\n";
            header->valid = false;
            return;
        }
        component->quantizationTableID = inFile.get();
        if (component->quantizationTableID > 3)
        {
            cout << "Error: Invalid quantization table ID in frame components\n";
            header->valid = false;
            return;
        }
    }
    if (length - 8 - (3 * header->numComponents) != 0)
    {
        cout << "Error: SOF Invalid\n";
        header->valid = false;
    }
}

void readQuantizationTable(ifstream &inFile, Header *const header)
{
    cout << "Reading DQT markers\n";
    int length = (inFile.get() << 8) + inFile.get(); // this was uint, but we turned it to signed (read next line)
    length -= 2;

    while (length > 0) // since in our length > 0 condition, length is uint, if it tried to go into negative, it will become a very large number and continue to remain true
    {
        byte tableInfo = inFile.get();
        length -= 1;
        byte tableID = tableInfo & 0x0F; // extracting the lowe 4 bits to get the tableID

        if (tableID > 3)
        {
            cout << "Error: invalid quantization table ID: " << (uint)tableID << endl;
            header->valid = false;
            return;
        }
        // we have checked that the tableID's are valid
        header->quantizationTables[tableID].set = true;
        if (tableInfo >> 4 != 0) // if the upper nibble of the tableinfo XX is non zero
        {
            // this is a 16 bit quantization table
            for (uint i = 0; i < 64; i++)
            {
                header->quantizationTables[tableID].table[zigZagMap[i]] = (inFile.get() << 8) + inFile.get();
            }
            length -= 128; // 64 values * 16 bit
        }
        else
        {
            // 8 bit quantization table
            for (uint i = 0; i < 64; i++)
            {
                header->quantizationTables[tableID].table[zigZagMap[i]] = inFile.get();
            }
            length -= 64; // 64 values * 8 bit
        }
    }
    if (length != 0) // that means it went into negative
    {
        cout << "DQT invalid\n";
        header->valid = false;
    }
}

void readAPPN(ifstream &inFile, Header *const header)
{
    // const just makes sure the header does not point to anything else, we can still make changes to its contents
    cout << "Reading APPN markers\n";
    uint length = (inFile.get() << 8) + inFile.get(); // this is in big endian

    for (uint i = 0; i < length - 2; i++) // -2 cuz we read the first 2
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

    last = inFile.get();
    current = inFile.get();

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

        // READING START OF FRAME
        if (current == SOF0)
        {
            header->frameType = SOF0;
            readStartOfFrame(inFile, header);
            break;
        }
        // READING QUANTUZATION TABLES
        else if (current == DQT)
        {
            readQuantizationTable(inFile, header);
        }

        else if (current >= APP0 && current <= APP15)
        {
            readAPPN(inFile, header); // function whole sole resp is to read a file with that marker and store it in the header
        }

        last = inFile.get();
        current = inFile.get();
    }
    return header;
}

void printHeader(const Header *const header)
{
    if (header == nullptr)
        return;
    cout << "DQT ---";
    // print the 4 quantization tables
    for (uint i = 0; i < 4; i++)
    {
        // check if that table is read
        if (header->quantizationTables[i].set)
        {
            cout << "Table ID: " << i << endl;
            cout << "Table data: " << endl;
            for (uint j = 0; j < 64; j++)
            {
                // pritn a newline for every 8th value
                if (j % 8 == 0)
                    cout << endl;
                cout << header->quantizationTables[i].table[j] << " ";
            }
            cout << endl;
        }
    }
    cout << "\nSOF --";
    cout << "Frame type: 0x" << hex << (uint)header->frameType << dec << "\n";
    cout << "Height: " << header->height << "\n";
    cout << "Width: " << header->width << "\n";
    cout << "Color components: \n";
    for (uint i = 0; i < header->numComponents; i++)
    {
        cout << "\nComponent ID: " << (i + 1) << "\n";
        cout << "Horizontal Sampling Factor: " << (uint)header->colorComponents[i].horizontalSamplingFactor << "\n";
        cout << "Vertical Sampling Factor: " << (uint)header->colorComponents[i].verticalSamplingFactor << "\n";
        cout << "Quantization Table ID: " << (uint)header->colorComponents[i].quantizationTableID << "\n";
    }
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
        if (header->valid == false)
        {
            cout << "Error: Invalid JPG\n";
            delete header;
            continue;
        }

        printHeader(header);

        // Decode the huffman data
        delete header;
    }
    return 0;
}
