#ifndef MIME_PARSER_H
#define MIME_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of a MIME boundary delimiter, including leading "--". */
#define MIME_MAX_BOUNDARY_LENGTH 128

/** Maximum length of an attachment filename. */
#define MIME_MAX_FILENAME_LENGTH 256

/**
 * @brief Locate a @c boundary= directive in a header line.
 *
 * On match, writes "--" followed by the boundary value into @p boundary_out.
 *
 * @param header_line       NUL-terminated header line to scan.
 * @param boundary_out      Buffer for the resulting "--<boundary>".
 * @param boundary_capacity Capacity of @p boundary_out.
 * @return true if a boundary was extracted, false otherwise.
 */
bool mime_find_boundary(const char *header_line,
                        char *boundary_out,
                        size_t boundary_capacity);

/**
 * @brief Locate a @c filename= directive in a header line.
 *
 * Strips surrounding quotes and trailing CR/LF from the value.
 *
 * @param header_line       NUL-terminated header line to scan.
 * @param filename_out      Buffer for the extracted filename.
 * @param filename_capacity Capacity of @p filename_out.
 * @return true if a filename was extracted, false otherwise.
 */
bool mime_find_filename(const char *header_line,
                        char *filename_out,
                        size_t filename_capacity);

/**
 * @brief Test whether a line begins a MIME header continuation.
 *
 * Used to skip metadata lines between the filename header and the
 * Base64-encoded body of an attachment.
 *
 * @param line NUL-terminated line to test.
 * @return true if @p line is a MIME header / continuation, false otherwise.
 */
bool mime_is_header_line(const char *line);

/**
 * @brief Walk a downloaded RFC 822 message and decode every attachment.
 *
 * Reads from @p message_file line by line, finds the first multipart
 * boundary, then extracts each base64-encoded attachment to a file in
 * the current working directory under its declared filename.
 *
 * @param message_file Open file handle positioned at the start of the message.
 * @return Number of attachments extracted, or -1 on parse error.
 */
int mime_extract_attachments(FILE *message_file);

#ifdef __cplusplus
}
#endif

#endif
