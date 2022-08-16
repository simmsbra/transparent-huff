#include <stdio.h>

int main(void) {
    FILE *file_out = fopen("one-of-each-byte-ascending", "w");
    for (int i = 0; i < 256; i += 1) {
        fputc(i, file_out);
    }
    fclose(file_out);

    return 0;
}
