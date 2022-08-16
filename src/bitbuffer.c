// see bitbuffer.h for an explanation of what the bit buffer is and how it is
// used

#include "bitbuffer.h"

// append the given bits to the end (right) of the buffer
void buffer_append_bits(
    struct bit_buffer *buffer,
    const bool *bits,
    int number_of_bits
) {
    for (int i = 0; i < number_of_bits; i += 1) {
        buffer->bits[i + buffer->length] = bits[i];
    }
    buffer->length += number_of_bits;
}

// append the given bit to the end (right) of the buffer
void buffer_append_bit(struct bit_buffer *buffer, bool bit) {
    bool bits[1] = {bit};
    buffer_append_bits(buffer, bits, 1);
}

// append the given byte to the end (right) of the buffer
void buffer_append_byte(struct bit_buffer *buffer, unsigned char byte) {
    bool bits[8];
    convert_byte_to_bits(byte, bits);
    buffer_append_bits(buffer, bits, 8);
}

// drop the given number of bits from the beginning (left) of the buffer
void buffer_drop_left_bits(struct bit_buffer *buffer, int number_of_bits) {
    assert(number_of_bits <= buffer->length);
    for (int i = 0; i < buffer->length - number_of_bits; i += 1) {
        buffer->bits[i] = buffer->bits[i + number_of_bits];
    }
    buffer->length -= number_of_bits;
}

// drop as many 8-bit slices as possible from the beginning (left) of the buffer
// and write them to the file as bytes
//
// afterward, the buffer will be left with 0 to 7 bits
void buffer_write_any_complete_bytes(FILE *fout, struct bit_buffer *buffer) {
    while (buffer->length >= 8) {
        fputc(convert_bits_to_byte(buffer->bits, 8), fout);
        buffer_drop_left_bits(buffer, 8);
    }
}

// assume buffer_write_any_complete_bytes() has already been called so that the
// buffer has 0 to 7 bits in it
//
// if there is at least 1 bit in the buffer, drop the remaining 1 to 7 bits from
// the buffer and write them to the file as 1 byte that is constructed by
// convert_bits_to_byte()
//
// afterward, the buffer will empty
void buffer_write_any_leftover_bits_as_byte(
    FILE *fout,
    struct bit_buffer *buffer
) {
    assert(buffer->length < 8);
    if (buffer->length > 0) {
        fputc(convert_bits_to_byte(buffer->bits, buffer->length), fout);
        buffer->length = 0;
    }
}

// return the given 1 to 8 bits as 1 byte that is padded on the right with one
// or more bits of value zero
//
// for example, if given the 2 bits {false, true}, then 0b01000000 would be the
// byte returned
unsigned char convert_bits_to_byte(const bool *bits, int number_of_bits) {
    assert(number_of_bits >= 1 && number_of_bits <= 8);

    unsigned char result = 0b00000000;
    for (int i = 0; i < number_of_bits; i += 1) {
        if (bits[i]) {
            // change the i'th bit in the byte from 0 to 1
            // example if "result" is 0b10000000 and "i" is 3:
            //     result:       0b10000000 (0)
            //     bit at i = 3: 0b00010000 (2^4) = (2^(7 - i))
            //     -------------------- ADD
            //     result:       0b10010000
            result += pow(2, 7 - i);
        }
    }
    return result;
}

// set each bit in the bit array of size 8 to what the bit is at the
// corresponding place in the byte
//
// for example, if given 0b11110001, then the bit array would become:
// {true, true, true, true, false, false, false, true}
void convert_byte_to_bits(unsigned char byte, bool *bits) {
    for (int i = 0; i < 8; i += 1) {
        // shift current bit all the way right to remove any bits to its right.
        // since the bit is now in the 2^0 (1) place, we can check if it is 1 by
        // seeing if the number is odd (not divisible by 2)
        // for example, if "i" = 3 and "byte" is 0b01110010
        //                                            ^
        // right shift by 4 or (7 - i)           0b00000111
        //                                                ^
        bits[i] = ((byte >> (7 - i)) % 2 == 1);
    }
}
