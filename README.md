# JPEG - Decoder

## How to run the program

- Navigate to the `/src` directory
- Run `g++ decoder.cxx`
- Run `a.exe ./pics/cat.jpg`

### Basic Overview of a JPEG encoder-decoder
Understanding a JPEG encoder. It consists of 4 major steps:
1. RGB -> YCbCr
- YCbCr color space separates luminance (Y) from chrominance (Cb and Cr), allowing for more efficient compression. Human vision is more sensitive to changes in brightness (luminance) than to changes in color (chrominance). By subsampling the chrominance channels (Cb and Cr), JPEG encoders can reduce the data needed to represent the image while preserving visual quality.
- The image is broken up into 8x8 blocks of pixels called MCUs (minimum coded units). Since we have 3 channels (Y, Cr, Cb), we will have 3 sets of MCUs. These MCUs can be treated as individual MCUs and further steps can be performed on them.

2. Discrete Cosine Transform (DCT)
- Converts image from spatial domain -> frequency domain
- low-frequency images? -> when pixels are very similar to their neighbors ; high-frequency images? -> when pixels are very different from their neighbors
- Human visual perception is less sensitive to changes in high-frequency information compared to low-frequency information
- The DCT tends to concentrate most of the image energy in a small number of low-frequency coefficients, while high-frequency coefficients represent finer details. Since the low-frequency coefficients contain a significant portion of the image's energy, they are less aggressively quantized during compression to preserve important image features and overall structure.

3. Quantization
- Reduces the precision of the frequency coefficients obtained after the Discrete Cosine Transform (DCT). By dividing the DCT coefficients by values in a quantization matrix, many of these coefficients become smaller and can be approximated to zero.
- Quantization is often designed to exploit properties of human visual perception. ie. high-frequency components -> larger quantization coefficients and lower will have smaller
- This is irreversible. eg: 23 and 5 => 23/5 = 4 => 4 * 5 = 20 (we lost precision data)
- These values decide how much your image is compressed.

4. Huffman Coding
- more frequent coeffs -> smaller code length ; less frequent coeffs -> larger code length
- is stored in a zig-zag fashion because the lower triangle consists of the high-frequency components which are highly quantized and are often 0. These 0s can be grouped for efficient storage. 


## Markers

### Start Of Image Marker (SOI)
Every JPEG file starts with SOI marker.
```
FFD8    - SOI Marker (2B)
```

### Define Restart Interval Marker (DRI)
The DC coefficient (which is essentially the first coefficient in an MCU) is dependent on the DC coefficient of the previous MCU (except the first MCU). What this essentially means is that, to calculate the actual MCU of say the second MCU, we add its value with the previous MCU's DC coefficient. 
DRI helps us define the interval of MCUs after which we reset the DC coefficient to zero.
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
[16]    - No of codes of each length [This is very important to generate the codes]
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

## Huffman Coding
### How do we figure out huffman codes from frequency of huffman codes of a particular length given in the DHT marker?
We start with the code ```0``` for codes of length 1 and at each step we do the following
* Save current symbol (```0``` in the case of codes of length 1) and add 1 to it
* For codes of length 2 we start with ```00``` and continue the same, adding a zero each time we consider a code of a greater length


