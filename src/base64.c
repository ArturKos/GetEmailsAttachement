#include "base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int decode_char(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int base64_decode(const char *input,
                  unsigned char *output,
                  size_t output_size,
                  size_t *bytes_written)
{
    if (input == NULL || output == NULL || bytes_written == NULL) {
        return -1;
    }

    size_t written = 0;
    int quartet[4];
    size_t filled = 0;
    int padding = 0;

    for (const char *p = input; *p != '\0'; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        if (c == '=') {
            quartet[filled++] = 0;
            ++padding;
        } else {
            int value = decode_char(c);
            if (value < 0 || padding > 0) {
                return -1;
            }
            quartet[filled++] = value;
        }
        if (filled == 4) {
            unsigned int triple =
                (unsigned int)quartet[0] << 18 |
                (unsigned int)quartet[1] << 12 |
                (unsigned int)quartet[2] << 6  |
                (unsigned int)quartet[3];
            size_t bytes_in_group = (size_t)(3 - padding);
            if (written + bytes_in_group > output_size) {
                return -1;
            }
            if (bytes_in_group >= 1) output[written++] = (triple >> 16) & 0xFF;
            if (bytes_in_group >= 2) output[written++] = (triple >> 8)  & 0xFF;
            if (bytes_in_group >= 3) output[written++] =  triple        & 0xFF;
            filled = 0;
            if (padding > 0) {
                break;
            }
        }
    }

    if (filled != 0) {
        return -1;
    }

    *bytes_written = written;
    return 0;
}

int base64_decode_file(const char *input_path, const char *output_path)
{
    FILE *input_file = fopen(input_path, "rb");
    if (input_file == NULL) {
        return -1;
    }

    if (fseek(input_file, 0, SEEK_END) != 0) {
        fclose(input_file);
        return -1;
    }
    long file_size = ftell(input_file);
    if (file_size < 0) {
        fclose(input_file);
        return -1;
    }
    rewind(input_file);

    char *encoded = malloc((size_t)file_size + 1);
    if (encoded == NULL) {
        fclose(input_file);
        return -1;
    }
    size_t read_bytes = fread(encoded, 1, (size_t)file_size, input_file);
    encoded[read_bytes] = '\0';
    fclose(input_file);

    size_t decoded_capacity = read_bytes; /* upper bound: decoded < encoded */
    unsigned char *decoded = malloc(decoded_capacity);
    if (decoded == NULL) {
        free(encoded);
        return -1;
    }

    size_t decoded_size = 0;
    int rc = base64_decode(encoded, decoded, decoded_capacity, &decoded_size);
    free(encoded);
    if (rc != 0) {
        free(decoded);
        return -1;
    }

    FILE *output_file = fopen(output_path, "wb");
    if (output_file == NULL) {
        free(decoded);
        return -1;
    }
    size_t written = fwrite(decoded, 1, decoded_size, output_file);
    free(decoded);
    if (fclose(output_file) != 0 || written != decoded_size) {
        return -1;
    }
    return 0;
}
