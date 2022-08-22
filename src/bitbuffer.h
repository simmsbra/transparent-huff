// we are working with bits, but the smallest amount of data that can be written
// to a file is a byte, so we need to have a bit buffer to keep adding bits to
// until the buffer length >= 8 and we can write at least 1 byte.
//
// in general, bits get appended to the end of the buffer and removed from the
// beginning of the buffer in increments of 8 when written to a file. if there
// are any leftover (less than 8) bits, they can be "flushed" by padding the
// right side with bits of 0 and writing the resulting byte to a file.
//
// also, the buffer is not meant to accumulate bits and then dump them when it
// is full -- instead, callers add only as many bits as they need to the buffer
// and then removed those bits as soon they don't need them anymore

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct bit_buffer {
    int length;
    // i haven't done real analysis on what the maximum possible amount of bits
    // in the buffer could be during any possible execution of the program, but
    // i think 512 is safe because it is about double what i think the maximum
    // possible number of bits is, which is 255 and happens during
    // decode_data_and_write() in decoder.c
    bool bits[512];
};

void buffer_append_bits(
    struct bit_buffer *buffer,
    const bool *bits,
    int number_of_bits
);
void buffer_append_bit(struct bit_buffer *buffer, bool bit);
void buffer_append_byte(struct bit_buffer *buffer, unsigned char byte);

void buffer_drop_left_bits(struct bit_buffer *buffer, int number_of_bits);

void buffer_write_any_complete_bytes(FILE *fout, struct bit_buffer *buffer);
void buffer_write_any_leftover_bits_as_byte(
    FILE *fout,
    struct bit_buffer *buffer
);

unsigned char convert_bits_to_byte(const bool *bits, int number_of_bits);
void convert_byte_to_bits(unsigned char byte, bool *bits);
