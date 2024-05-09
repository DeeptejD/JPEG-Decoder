#include <iostream>
#include <fstream>
#include "jpg.h"

// Declarations

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

// reads the Define Restart Interval (DRI) marker
void readRestartInterval(std::ifstream &inFile, Header *const header);

// reads the huffman tables
void readHuffmanTable(std::ifstream &inFile, Header *const header);

// reads start of scan marker
void readStartOfScan(std::ifstream &inFile, Header *const header);

// reads a comment
void readComment(std::ifstream &inFile, Header *const header);

// Definitions

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
        // Component ID's are usually 1, 2, 3 but rarely can be seen as 0, 1, 2
        // Always force them to 1, 2, 3 for consistency
        if (componentID == 0)
            header->zeroBased = true;
        if (header->zeroBased)
            componentID += 1;

        if (componentID == 4 || componentID == 5)
        {
            std::cout << "ERROR: YIQ color mode not supported\n";
            header->valid = false;
            return;
        }
        // Generally component ID's are 1, 2, 3. But some JPEG's (like gorilla.jpg) use CID's starting from 0
        if (componentID == 0 || componentID > 3)
        {
            std::cout << "Error:  Invalid component ID: " << (uint)componentID << "\n";
            header->valid = false;
            return;
        }

        ColorComponent *component = &header->colorComponents[componentID - 1]; // -1 because the component ID's are 1 indexed and the indices we need to read are zero indexed

        if (component->used)
        {
            std::cout << "ERROR: Duplicate color component ID\n";
            header->valid = false;
            return;
        }

        component->used = true;

        byte samplingFactor = inFile.get();
        // Sampling factor is also split into upper and lower nibble
        component->horizontalSamplingFactor = samplingFactor >> 4; // upper nibble
        component->verticalSamplingFactor = samplingFactor & 0x0F; // lower nibble
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

    // once we read all the color channels SOF shud ideally be over, but just to be sure
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
    std::cout << "\nDQT\n---\n";
    for (int i = 0; i < 4; i++) // all 4 quantization tables
    {
        if (header->quantizationTables[i].set)
        {
            std::cout << "\nTable ID: " << i << "\n";
            std::cout << "Table Data";

            for (uint j = 0; j < 64; j++)
            {
                if (j % 8 == 0)
                    std::cout << "\n";

                std::cout << header->quantizationTables[i].table[j] << "\t ";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\nStart of Frame\n--------------\n";
    std::cout
        << "Frame type:\t0x" << std::hex << (uint)header->frameType << std::dec << "\n";
    std::cout << "Height:\t\t" << header->height << "\n";
    std::cout << "Width:\t\t" << header->width << "\n";
    std::cout << "\nColor components\n";
    for (uint i = 0; i < header->numComponents; i++)
    {
        std::cout << "\nComponent ID:\t\t\t" << (i + 1) << "\n";
        std::cout << "Horizontal Sampling Factor:\t" << (uint)header->colorComponents[i].horizontalSamplingFactor << "\n";
        std::cout << "Vertical Sampling Factor:\t" << (uint)header->colorComponents[i].verticalSamplingFactor << "\n";
        std::cout << "Quantization Table ID:\t\t" << (uint)header->colorComponents[i].quantizationTableID << "\n";
    }

    // Printing the Restart Interval
    std::cout << "\nRestart Interval: " << header->restartInterval << "\n";

    std::cout << "\nDefine Huffman Tables (DHT)\n---------------------------\n";
    // Printing DC Huffman Tables
    std::cout << "DC Tables\n---------";
    for (uint i = 0; i < 4; i++)
    {
        if (header->huffmanDCTables[i].set)
        {
            std::cout << "\nTable ID: " << i << "\n";
            std::cout << "Symbols\n";
            for (uint j = 0; j < 16; j++)
            {
                std::cout << (j + 1) << ": ";
                for (uint k = header->huffmanDCTables[i].offset[j]; k < header->huffmanDCTables[i].offset[j + 1]; k++)
                {
                    std::cout << std::hex << (uint)header->huffmanDCTables[i].symbols[k] << std::dec << ' ';
                }
                std::cout << "\n";
            }
        }
    }

    // Printing AC Huffman Tables
    std::cout << "\n\nAC Tables\n---------";
    for (uint i = 0; i < 4; i++)
    {
        if (header->huffmanACTables[i].set)
        {
            std::cout << "\nTable ID: " << i << "\n";
            std::cout << "Symbols\n";
            for (uint j = 0; j < 16; j++)
            {
                std::cout << (j + 1) << ": ";
                for (uint k = header->huffmanACTables[i].offset[j]; k < header->huffmanACTables[i].offset[j + 1]; k++)
                {
                    std::cout << std::hex << (uint)header->huffmanACTables[i].symbols[k] << std::dec << ' ';
                }
                std::cout << "\n";
            }
        }
    }
    std::cout << "\nStart of Selection\n------------------\n";
    std::cout << "Start of Selection:\t\t" << (uint)header->startOfSelection << '\n';
    std::cout << "End of Selection:\t\t" << std::dec << (uint)header->endOfSelection << '\n';
    std::cout << "Successive Approximation High:\t" << (uint)header->successiveApproximationHigh << '\n';
    std::cout << "Successive Approximation Low:\t" << (uint)header->successiveApproximationLow << '\n';
    std::cout << "\nColor Components\n\n";
    for (uint i = 0; i < header->numComponents; ++i)
    {
        std ::cout << "Component ID:\t\t" << (i + 1) << '\n';
        std ::cout << "Huffman DC Table ID:\t" << (uint)header->colorComponents[i].HuffmanDCTableID << '\n';
        std::cout << "Huffman AC Table ID:\t" << (uint)header->colorComponents[i].HuffmanACTableID << '\n';
    }
    std ::cout << "Length of Huffman Data:\t" << header->huffmanData.size() << '\n';

    std::cout << "DRI=============\n";
    std::cout << "Restart Interval: " << header->restartInterval << '\n';
}

void readAPPN(std::ifstream &inFile, Header *const header)
{
    // const just makes sure the header does not point to anything else, we can still make changes to its contents
    std::cout << "Reading APPN Markers...\n";
    uint length = (inFile.get() << 8) + inFile.get(); // we are reading 2 bytes from the length part (remember - FFXX LLLL), left shifting by 8 cuz, firs read byte goes in the Sig pos (BIG ENDIAN)

    for (uint i = 0; i < length - 2; i++) // -2 cuz we read the first 2
        inFile.get();                     // we dont care about the APPN markers so we simply read them, this basically advances the position of the pointer
}

void readComment(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading COM marker\n";
    uint length = (inFile.get() << 8) + inFile.get();

    for (uint i = 0; i < length - 2; i++)
    {
        inFile.get();
    }
}

void readQuantizationTable(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading DQT Markers...\n";
    int length = (inFile.get() << 8) + inFile.get(); // NOTE: here length is not uint because we want to know if it goes below 0 for the while loop below
    length -= 2;

    // length is int not uint coz it can go negative in case of an error
    while (length > 0)
    {
        byte tableInfo = inFile.get();
        length--;

        // table id is the lower nibble of tableInfo
        byte tableID = tableInfo & 0x0F; // 0x0F is 15 (15 bits)

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
            // we are readnig 2 bytes here beacause since tableinfo is 1, we are reading a 16B quantization table
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

void readRestartInterval(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading DRI marker...\n";
    uint length = (inFile.get() << 8) + inFile.get();

    // setting the restart interval to the next 16bit integer
    header->restartInterval = (inFile.get() << 8) + inFile.get();

    // checking if the marker is valid
    if (length - 4 != 0) // subtracting 4 from the length since we read 4 bytes
    {
        std::cout << "Error: DRI Invalid\n";
        header->valid = false;
    }
}

Header *readJPG(const std::string &filename)
{
    // open file in binary
    std::ifstream inFile = std::ifstream(filename, std::ios::in | std::ios::binary);

    // error handling
    if (!inFile.is_open())
    {
        std::cout << "ERROR: Error opening input file\n";
        return nullptr;
    }

    // std::nothrow returns a nullpointer if in case the allocation were to fail, avoids try-catch
    Header *header = new (std::nothrow) Header;

    if (header == nullptr)
    {
        std::cout << "ERROR: Memory error\n";
        inFile.close();
        return nullptr;
    }

    // READING THE FILE STARTS HERE

    // since markers are 2 bytes long we read 2 bytes at a time
    byte last = inFile.get();
    byte current = inFile.get();

    // Checking if the first 2 bytes in the JPEG are valid (remember how a marker looks -- FFXX)
    if (last != 0xFF || current != SOI)
    {
        header->valid = false;
        inFile.close();
        return header;
    }

    last = inFile.get();
    current = inFile.get();

    // Read Markers
    while (header->valid) // we keep reading until we run out of markers or something else goes wrong
    {
        // check if we've reached past the end of the file
        if (!inFile)
        {
            std::cout << "ERROR: File ended prematurely\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        // since we expect a marker at the beginning of each iteration of the loop
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
        }
        else if (current == DQT)
        {
            // read quantization table marker
            readQuantizationTable(inFile, header);
        }
        else if (current == DHT)
        {
            readHuffmanTable(inFile, header);
        }
        else if (current == SOS)
        {
            readStartOfScan(inFile, header);
            // break from the while loop after SOS
            break;
        }
        else if (current == DRI)
        {
            // Test on: gorilla.jpg
            // Reads the restart interval marker (i.e. how often are the DC coefficients of the MCU's reset)
            readRestartInterval(inFile, header);
        }
        else if (current >= APP0 && current <= APP15)
        {
            // read appn marker
            readAPPN(inFile, header);
        }
        else if (current == COM) // comment
        {
            readComment(inFile, header);
        }
        // following are some unused markers that can be skipped
        else if ((current >= JPG0 && current <= JPG11) || current == DNL || current == DHP || current == EXP)
        {
            readComment(inFile, header);
        }
        else if (current == TEM)
        {
            // this has no size, so we dont do anything
        }
        // Handling any number of FF's in a row
        // any number of FF's are allowed and must be ignored
        else if (current == 0xFF)
        {
            current = inFile.get();
            continue;
        }
        else if (current == SOI)
        {
            std::cout << "Error: Embedded JPG's not supported\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        else if (current == EOI)
        {
            std::cout << "Error: EOI detected before SOS\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        else if (current == DAC)
        {
            std::cout << "Error: Arithmetic code not supported\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        else if (current >= SOF0 && current <= SOF15)
        {
            std::cout << "Error: SOF marker not supported: 0x" << std::hex << (uint)current << std::dec << "\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        else if (current >= RST0 && current <= RST7)
        {
            std::cout << "Error: RSTN deteted before SOS\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        else
        {
            std::cout << "Error: Unknown Marker: 0x" << std::hex << (uint)current << std::dec << "\n";
            header->valid = false;
            inFile.close();
            return header;
        }

        last = inFile.get();
        current = inFile.get();
    }

    // after SOS
    if (header->valid)
    {
        current = inFile.get();
        // read compressed image data
        while (true)
        {
            if (!inFile)
            {
                std::cout << "Error: File ended prematurely\n";
                header->valid = false;
                inFile.close();
                return header;
            }

            last = current;
            current = inFile.get();

            // if marker is found
            if (last == 0xFF)
            {
                // end of image
                if (current == EOI)
                {
                    break;
                }
                else if (current == 0x00)
                {
                    header->huffmanData.push_back(last);
                    current = inFile.get();
                }
                // restart marker
                else if (current >= RST0 && current <= RST7)
                {
                    current = inFile.get();
                }
                // ignore multiple 0xFF's in a row
                else if (current == 0xFF)
                {
                    // do nothing
                    continue;
                }
                else
                {
                    std::cout << "Error: Invalid marker during compressed data scan. 0x" << std::hex << (uint)current << std::dec << "\n";
                    header->valid = false;
                    inFile.close();
                    return header;
                }
            }
            else
            {
                // this is if last is nto equal to 0xFF
                header->huffmanData.push_back(last);
            }
        }
    }

    // validate header info before returning
    if (header->numComponents != 1 && header->numComponents != 3)
    {
        std ::cout << "Error - " << (uint)header->numComponents << " color components given (1 or 3 required)\n";
        header->valid = false;
        inFile.close();
        return header;
    }

    for (uint i = 0; i < header->numComponents; ++i)
    {
        if (header->quantizationTables[header->colorComponents[i].quantizationTableID].set == false)
        {
            std::cout << "Error - Color component using uninitialized quantization table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        if (header->huffmanDCTables[header->colorComponents[i].HuffmanDCTableID].set == false)
        {
            std::cout << "Error - Color component using uninitialized Huffman DC table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
        if (header->huffmanACTables[header->colorComponents[i].HuffmanACTableID].set == false)
        {
            std::cout << "Error - Color component using uninitialized Huffman AC table\n";
            header->valid = false;
            inFile.close();
            return header;
        }
    }

    inFile.close();
    return header;
}

void readHuffmanTable(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading DHT Marker...\n";
    int length = (inFile.get() << 8) + inFile.get();
    length -= 2; // since we already read 2 bytes

    while (length > 0)
    {
        byte tableInfo = inFile.get();
        byte tableID = tableInfo & 0x0F; // get the lower nibble
        bool ACTable = tableInfo >> 4;   // get the upper nibble

        if (tableID > 3)
        {
            std::cout << "Error: Invalid Huffman Table with table ID: " << (uint)tableID << "\n";
            header->valid = false;
            return;
        }

        HuffmanTable *hTable;
        if (ACTable)
            hTable = &header->huffmanACTables[tableID];
        else
            hTable = &header->huffmanDCTables[tableID];
        hTable->set = true;

        hTable->offset[0] = 0;
        uint allSymbols = 0;

        for (uint i = 1; i <= 16; i++)
        {
            allSymbols += inFile.get();
            hTable->offset[i] = allSymbols;
        }

        if (allSymbols > 162)
        {
            std::cout << "Error: Too many symbols in the Huffman Table\n";
            header->valid = false;
            return;
        }

        // reading the next chunk
        for (uint i = 0; i < allSymbols; i++)
        {
            hTable->symbols[i] = inFile.get();
        }

        length -= 17 + allSymbols;
    }
    if (length != 0)
    {
        std::cout << "Error: DHT Invalid\n";
        header->valid = false;
    }
}

void readStartOfScan(std::ifstream &inFile, Header *const header)
{
    std::cout << "Reading of Scan Marker...\n";
    // We should not run into the SOS marker before reading the SOF marker
    // We can detect an error by checking if the number of components is still zero
    // It must be a non-zero value if we have gone through SOF marker before
    if (header->numComponents == 0)
    {
        std::cout << "Error: SOS detected before SOF\n";
        header->valid = false;
        return;
    }
    uint length = (inFile.get() << 8) + inFile.get();

    // Setting all the used flags back to false coz we want to use them below
    for (uint i = 0; i < header->numComponents; i++)
    {
        header->colorComponents[i].used = false;
    }

    byte numComponents = inFile.get();
    for (uint i = 0; i < numComponents; i++)
    {
        byte componentID = inFile.get();
        if (header->zeroBased)
            componentID += 1;

        if (componentID > header->numComponents)
        {
            std::cout << "Error: Invalid color component ID: " << (uint)componentID << "\n";
            header->valid = false;
            return;
        }

        ColorComponent *component = &header->colorComponents[componentID - 1];

        // if we run into a component whose used flag is true, this means we encountered this component twice in this loop
        if (component->used)
        {
            std::cout << "Error: Duplicate color component ID: " << (uint)componentID << "\n";
            header->valid = false;
            return;
        }
        component->used = true;

        // reading the second byte which is the huffman table IDs
        byte huffmanTableIDs = inFile.get();
        // upper nibble is the DC table ID
        component->HuffmanDCTableID = huffmanTableIDs >> 4;
        // lower nibble is the AC table ID
        component->HuffmanACTableID = huffmanTableIDs & 0x0F;
        if (component->HuffmanDCTableID > 3)
        {
            std::cout << "Error: Invalid Huffman DC table ID: " << (uint)component->HuffmanDCTableID << "\n";
            header->valid = false;
            return;
        }
        if (component->HuffmanACTableID > 3)
        {
            std::cout << "Error: Invalid Huffman AC table ID: " << (uint)component->HuffmanACTableID << "\n";
            header->valid = false;
            return;
        }
    }
    header->startOfSelection = inFile.get();
    header->endOfSelection = inFile.get();
    byte successiveApproximation = inFile.get();
    header->successiveApproximationHigh = successiveApproximation >> 4;
    header->successiveApproximationLow = successiveApproximation & 0x0F;

    // Verifying that these values are 0, 63, 0, 0
    // Baseline JPGs don't use spectral selection and successive approximation
    if (header->startOfSelection != 0 || header->endOfSelection != 63)
    {
        std::cout << "Error: Invalid spectral selection\n";
        header->valid = false;
        return;
    }
    if (header->successiveApproximationHigh != 0 || header->successiveApproximationLow != 0)
    {
        std::cout << "Error: Invalid successive approximation\n";
        header->valid = false;
        return;
    }

    // Verifying that the length we read is correct based on the number of bytes we read
    if (length - 6 - (2 * numComponents) != 0)
    {
        std::cout << "Error SOS Invalid\n";
        header->valid = false;
    }
}
