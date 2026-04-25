#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode a Base64-encoded string into a binary output buffer.
 *
 * Skips whitespace and newlines; rejects characters outside the standard
 * Base64 alphabet (RFC 4648, no URL-safe variant).
 *
 * @param input        NUL-terminated Base64 input.
 * @param output       Caller-provided buffer for decoded bytes.
 * @param output_size  Capacity of @p output in bytes.
 * @param bytes_written Out-parameter receiving the number of bytes written.
 * @return 0 on success, -1 on malformed input or insufficient buffer.
 */
int base64_decode(const char *input,
                  unsigned char *output,
                  size_t output_size,
                  size_t *bytes_written);

/**
 * @brief Decode an entire Base64 file to a binary file.
 *
 * Streams @p input_path through @ref base64_decode in chunks and writes
 * the decoded bytes to @p output_path.
 *
 * @param input_path   Path to a file containing Base64 text.
 * @param output_path  Destination path for the decoded binary.
 * @return 0 on success, -1 on I/O or decode error.
 */
int base64_decode_file(const char *input_path, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif
