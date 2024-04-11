# JPEG - Encoder & Decoder

## How to run the program

- Navigate to the `/src` directory
- Run `g++ decoder.cxx`
- Run `a.exe ./pics/cat.jpg`

## Markers

### Define Restart Interval Marker (DRI)
The DC coefficient (which is essentially the first coefficient in an MCU) is dependent on the DC coefficient of the previous MCU (except the first MCU). What this essentially means is that, to calculate the actual MCU of say the second MCU, we add its value with the previous MCU's DC coefficient. 
DRI helps us define the interval of MCU's after which we reset the DC coefficient to zero.
```
FFDD    - Marker (2B)
0004    - Length (2B)
XXXX    - Restart Interval (2B)
```

### Define Huffman Tables Marker (DHT)
```
FFC4    - Marker (2B)
XXXX    - Length (2B)
XX      - Table Info (1B)
[16]    - No of codes of each length
[X]     - Symbols (Sum of the prev 16)
```
Structure of the Table Info symbol (1byte)
| Upper Nibble                                                        | Lower Nibble                                                     |
|---------------------------------------------------------------------|------------------------------------------------------------------|
| Boolean Value Either 0/1  0 - AC Huffman Table 1 - DC Huffman Table | Table ID   Values from 0-3 (Not shared between AC and DC tables) |

### Start of Scan Marker (SoS)
```
FFDA    - Marker (2B)
XXXX    - Length (2B)
XX      - Number of Color Components (1B)
XX      - Component ID (1B)
XX      - DC/AC Table ID (1B)

XX      - Start of Selection (1B) [This value must be zero for Baseline JPEGs]
XX      - End of Selection (1B) [This value must be 63 for Baseline JPEGs]
XX      - Successive Approximation (1B) [Relevant w.r.t. Progressive JPEGs, must be 0]

---
This is followed by the Huffman Coded BitSteam
```


xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

### Bit Map (BMP) Header [For the bmp output]
```
First Half Of The Header [14B]:
"BM"           - A start notation (2B)
size           - Header Size (4B)
00 00 00 00    - 4 unused bytes (4B)
00 00 00 1A    - Pixel Array Offset from start of the file (4B) [will always be the said num., because of the header options we're using]

Size Break-up = 
14 (first 1/2)
+ 12 (second 1/2)
+ H*W*3 (height * width * numOfColorChannels)
+ P * H (padding bytes * num of rows (ie. height)

Second Half Of The Header (DIB Header) [12B]:
12             - DIB Header Size (4B) [always 12]
width          - image width (2B)
height         - image height (2B)
1              - number of planes (2B) [must be 1]
24             - number of bits per pixel (2B) [8 (bits per color channel 0->255) * 3 (num of color components)]
```

