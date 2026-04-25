#include "mime_parser.h"
#include "base64.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void copy_until_crlf(const char *source,
                            char *destination,
                            size_t capacity,
                            bool strip_quotes_and_spaces)
{
    size_t output_index = 0;
    for (const char *p = source; *p != '\0' && *p != '\r' && *p != '\n'; ++p) {
        if (strip_quotes_and_spaces && (*p == '"' || *p == ' ')) {
            continue;
        }
        if (output_index + 1 >= capacity) {
            break;
        }
        destination[output_index++] = *p;
    }
    destination[output_index] = '\0';
}

bool mime_find_boundary(const char *header_line,
                        char *boundary_out,
                        size_t boundary_capacity)
{
    if (header_line == NULL || boundary_out == NULL || boundary_capacity < 3) {
        return false;
    }
    const char *match = strstr(header_line, "boundary=");
    if (match == NULL) {
        return false;
    }
    match += strlen("boundary=");

    char raw[MIME_MAX_BOUNDARY_LENGTH];
    copy_until_crlf(match, raw, sizeof(raw), true);
    if (raw[0] == '\0') {
        return false;
    }

    int written = snprintf(boundary_out, boundary_capacity, "--%s", raw);
    return written > 0 && (size_t)written < boundary_capacity;
}

bool mime_find_filename(const char *header_line,
                        char *filename_out,
                        size_t filename_capacity)
{
    if (header_line == NULL || filename_out == NULL || filename_capacity == 0) {
        return false;
    }
    const char *match = strstr(header_line, "filename=");
    if (match == NULL) {
        return false;
    }
    match += strlen("filename=");

    copy_until_crlf(match, filename_out, filename_capacity, true);
    if (filename_out[0] == '\0') {
        return false;
    }

    /* Replace path separators and control characters that are unsafe as
     * filenames in the current working directory. */
    for (char *p = filename_out; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\' || (unsigned char)*p < 32) {
            *p = '_';
        }
    }
    return true;
}

bool mime_is_header_line(const char *line)
{
    if (line == NULL) {
        return false;
    }
    if (strncmp(line, "Content-", 8) == 0) {
        return true;
    }
    char first = line[0];
    return first == '\0' || first == '\r' || first == '\n' ||
           first == ' '  || first == '\t' || first == '=';
}

static bool starts_with(const char *line, const char *prefix)
{
    return strncmp(line, prefix, strlen(prefix)) == 0;
}

static bool is_boundary_terminator(const char *line, const char *boundary)
{
    size_t boundary_length = strlen(boundary);
    if (strncmp(line, boundary, boundary_length) != 0) {
        return false;
    }
    return line[boundary_length] == '-' && line[boundary_length + 1] == '-';
}

static int read_logical_line(FILE *file, char *line, size_t capacity)
{
    if (fgets(line, (int)capacity, file) == NULL) {
        return -1;
    }
    size_t length = strlen(line);
    while (length > 0 && (line[length - 1] == '\r' || line[length - 1] == '\n')) {
        line[--length] = '\0';
    }
    return (int)length;
}

static int decode_attachment(const char *base64_path, const char *output_path)
{
    return base64_decode_file(base64_path, output_path);
}

int mime_extract_attachments(FILE *message_file)
{
    if (message_file == NULL) {
        return -1;
    }

    char line[1024];
    char boundary[MIME_MAX_BOUNDARY_LENGTH] = {0};
    char filename[MIME_MAX_FILENAME_LENGTH] = {0};
    bool boundary_found = false;
    bool filename_found = false;
    FILE *base64_sink = NULL;
    char base64_path[] = "/tmp/pop3_b64_XXXXXX";
    int extracted = 0;

    while (read_logical_line(message_file, line, sizeof(line)) >= 0) {
        if (!boundary_found) {
            boundary_found = mime_find_boundary(line, boundary, sizeof(boundary));
            continue;
        }
        if (!filename_found) {
            if (mime_find_filename(line, filename, sizeof(filename))) {
                int temp_fd = mkstemp(base64_path);
                if (temp_fd < 0) {
                    return -1;
                }
                base64_sink = fdopen(temp_fd, "w+b");
                if (base64_sink == NULL) {
                    close(temp_fd);
                    unlink(base64_path);
                    return -1;
                }
                filename_found = true;
            }
            continue;
        }
        if (is_boundary_terminator(line, boundary)) {
            if (base64_sink != NULL) {
                fclose(base64_sink);
                base64_sink = NULL;
            }
            if (decode_attachment(base64_path, filename) == 0) {
                ++extracted;
            }
            unlink(base64_path);
            filename_found = false;
            filename[0] = '\0';
            /* Reset the template so mkstemp will accept it again. */
            strcpy(base64_path, "/tmp/pop3_b64_XXXXXX");
            break;
        }
        if (starts_with(line, boundary)) {
            if (base64_sink != NULL) {
                fclose(base64_sink);
                base64_sink = NULL;
            }
            if (filename[0] != '\0') {
                if (decode_attachment(base64_path, filename) == 0) {
                    ++extracted;
                }
                unlink(base64_path);
            }
            filename_found = false;
            filename[0] = '\0';
            strcpy(base64_path, "/tmp/pop3_b64_XXXXXX");
            continue;
        }
        if (mime_is_header_line(line)) {
            continue;
        }
        if (base64_sink != NULL) {
            fputs(line, base64_sink);
        }
    }

    if (base64_sink != NULL) {
        fclose(base64_sink);
        unlink(base64_path);
    }
    return extracted;
}
