# transparent-huff
transparent-huff implements, in C, Huffman coding for file compression in a way that allows you to see the data structures used for encoding. This, along with the easy-to-follow source code, can hopefully give you a better idea of how Huffman coding works and how it may be implemented in a non-memory-managed language.

## Preview
```
Input File Contents:
sleeveless lee sees sleeves

Byte Frequencies:
 32 ( ): 3
101 (e): 11
108 (l): 4
115 (s): 7
118 (v): 2

Huffman Tree:
27
├ 16
│ ├ 9
│ │ ├ 5
│ │ │ ├ 3:  32 ( )
│ │ │ └ 2: 118 (v)
│ │ └ 4: 108 (l)
│ └ 7: 115 (s)
└ 11: 101 (e)

Prefix Code (Symbol-to-Codeword Mappings):
 32 ( ): 0000
101 (e): 1
108 (l): 001
115 (s): 01
118 (v): 0001

Results:
The compressed file is 70% the size of the original file.
The original file and the decompressed file match.
```

## Purpose
- Get experience writing and debugging bit-level software
- Work out my C muscles, including non-trivial pointer usage and declarations
- Work out my recursion muscles
- Provide straightforward and well-written(?) source code as a reference for people working on similar projects

## Usage (Linux)
  1. Make sure you have `gcc` installed (for compiling the C source files).
  2. Download or clone this repository and move into its directory.
  3. Run `./build.sh` to compile the source into the `encoder` and `decoder` binaries.
### With Helper Script
  4. Run the compression-and-decompression script with any file.
     - `./compress-then-decompress.sh sample-files/slss`
### Without Helper Script
  - Both the `encoder` and `decoder` binaries read from the given filename, output messages to `stderr`, and output data to `stdout`. By default, both `stderr` and `stdout` get printed to the terminal, but in these steps we will separate the streams by redirecting `stdout` to a file. Note that this will overwrite the file you are redirecting to.
  4. Compress any file, redirecting `stdout` to your desired filename.
     - `./encoder sample-files/slss > slss.compressed`
  5. Decompress the compressed file, redirecting `stdout` to your desired filename.
     - `./decoder slss.compressed > slss.decompressed`

## Notes
- When compressing very small files, the compressed file is actually bigger than the original file because the encoded data plus the metadata needed to decode it (which is the number of bytes encoded and the Huffman tree) takes up more bytes than the original data itself.
- When compressing a file that has only 1 unique byte/symbol, an extra, arbitrary node is added to maintain the fact that the Huffman tree is a binary tree, since that is what the related functions operate on. Otherwise, logic would be needed to also handle 1-node "trees".
- You can quickly make your own sample file without a newline character at the end by running something like `echo -n "alfalfa" > filename-here` on Linux. Note that this will overwrite the file if it already exists.
- I was about to make a test file that forced the maximum codeword length of 255, but if my reasoning and math are correct, that file would have this many bytes: 1 + sum 2^i, i=0 to 254
- To fully understand the source code, you should have a basic idea of how Huffman coding works. One way you can learn is by watching the video in the "Thanks" section below.
- For some functions, I used declarations like `int function(int array[256])` instead of `int function(int *array)` to make it clear that the array is expected to have exactly that many elements, even though the argument just decays to a pointer anyway.
- I used Valgrind to fix any memory leaks I could find. I did this by testing each return branch in the main functions of both `src/encoder.c` and `src/decoder.c`. I don't know if that is sufficient to say that there are no possible memory leaks, though.
- These programs are definitely not the fastest nor the most memory efficient, but that's OK since they weren't designed to be.

## Author
This project was created by Brandon Simmons.

## Thanks
Thank you, Marty Stepp, for explaining in this video how Huffman coding works: [42 CS 106B Lecture Huffman Encoding in detail](https://www.youtube.com/watch?v=7pIHeffQXjw).

Thank you, David Dailey, for making [this webpage](http://srufaculty.sru.edu/david.dailey/words/longrepeats.html) of words that are long but have few unique letters. I used some of those words here as examples.

Thank you, countless people whose explanations I read online while researching things about C and Huffman coding.

And thank you for visiting.
