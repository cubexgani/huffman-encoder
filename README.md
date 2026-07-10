# Huffman Encoder + Decoder

[Huffman Encoding](https://en.wikipedia.org/wiki/Huffman_coding) is a data compression technique which is lossless. This means, when a text is encoded and then decoded, we should get back the exact same text. Its basic principle is that more frequent characters should have less number of bits in its Huffman code. This project implements this Huffman Encoder and Decoder in ANSI C.

## Features
1. The encoder and the decoder can be used completely independent of each other. No data is shared between the two programs.
2. Tree is serialized in the encoded data file (along with the text).
3. File extension of encoded data file doesn't matter; magic bytes written in the file header are validated in the decoder before proceeding further.
4. Safeguards are implemented so that corrupted data files are reported.

### Encoded file format
`[ Magic bytes ][ Tree length in bits (4 bytes) ][ Serialized tree ][ Text length in bits (4 bytes) ][ Encoded text ]`
Here magic bytes: `00 1a 48 55 46 46 ff 0a` (`48 55 46 46` corresponds to "HUFF" in ASCII)

## Requirements
1. Linux (or WSL) (because `unistd.h` is being used). I think MinGW works too.
2. A C compiler. Anything but MSVC.

## Usage
1. Generate executables, one for each file.
2. The encoder can be used in 2 ways:
	- `./huffman_encoder output_filename` (text input is taken through stdin)
	- `cat test_file.txt | ./huffman_encoder output_filename` (file contents are parsed)
3. Decoder usage: `./huffman_decoder input_filename output_filename`

## Notes
I would like to take a moment to list down top 10 reasons I wanted to flex this project:
1. Only 2 files for this entire project: one for the encoder, another for the decoder.
2. Entire project is written in vim editor with minimal customization (line number and color scheme).
3. Entire project is written in ANSI C.
4. No build managers like cmake is used.
5. I serialized and deserialized the huffman tree correctly.
6. Implemented a dynamic byte buffer, along with a buffer reader. This and the serialization and deserialization were harder than implementing the Huffman coding logic for me.
7. No memory crumbs left behind; all blocks are freed hehehehe (thanks `valgrind`)
8. Size of compressed file feels soo damn good after comparing with the size of the original file.
9. It feels even better when the text is decoded correctly.
10. Terminal colors are used to display the encoded bytes and the lookup table.

That being said, there are still a few **to-dos**:
1. Make the source codes more readable.
2. Structures for error handling (maybe).

