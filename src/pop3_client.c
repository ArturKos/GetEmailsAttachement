#include "pop3_client.h"
#include "mime_parser.h"

#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static bool send_all(int socket_fd, const char *data, size_t length)
{
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(socket_fd, data + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return false;
        }
        total_sent += (size_t)sent;
    }
    return true;
}

static bool send_command(int socket_fd, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

static bool send_command(int socket_fd, const char *format, ...)
{
    char command[512];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(command, sizeof(command), format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        return false;
    }
    return send_all(socket_fd, command, (size_t)written);
}

static ssize_t recv_line(int socket_fd, char *buffer, size_t capacity)
{
    if (capacity < 2) {
        return -1;
    }
    ssize_t received = recv(socket_fd, buffer, capacity - 1, 0);
    if (received <= 0) {
        return -1;
    }
    buffer[received] = '\0';
    return received;
}

static bool response_is_ok(const char *response)
{
    return strncmp(response, "+OK", 3) == 0;
}

int pop3_connect(const char *server_host, const char *server_port)
{
    if (server_host == NULL) {
        return -1;
    }
    const char *port = (server_port != NULL) ? server_port : POP3_DEFAULT_PORT;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *resolved = NULL;
    if (getaddrinfo(server_host, port, &hints, &resolved) != 0) {
        return -1;
    }

    int socket_fd = -1;
    for (struct addrinfo *candidate = resolved; candidate != NULL; candidate = candidate->ai_next) {
        socket_fd = socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }
        if (connect(socket_fd, candidate->ai_addr, candidate->ai_addrlen) == 0) {
            break;
        }
        close(socket_fd);
        socket_fd = -1;
    }
    freeaddrinfo(resolved);

    if (socket_fd < 0) {
        return -1;
    }

    char greeting[POP3_RECV_BUFFER_SIZE];
    if (recv_line(socket_fd, greeting, sizeof(greeting)) <= 0 || !response_is_ok(greeting)) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

bool pop3_login(int socket_fd, const char *username, const char *password)
{
    if (username == NULL || password == NULL) {
        return false;
    }
    char response[POP3_RECV_BUFFER_SIZE];

    if (!send_command(socket_fd, "USER %s\r\n", username)) {
        return false;
    }
    if (recv_line(socket_fd, response, sizeof(response)) <= 0 || !response_is_ok(response)) {
        return false;
    }

    if (!send_command(socket_fd, "PASS %s\r\n", password)) {
        return false;
    }
    if (recv_line(socket_fd, response, sizeof(response)) <= 0 || !response_is_ok(response)) {
        return false;
    }
    return true;
}

/**
 * Read a multi-line POP3 response, terminated by a line containing only ".".
 * Counts the number of message-body lines and writes the entire response into
 * @p destination_file (callers that don't need the body pass NULL).
 */
static int read_multiline_response(int socket_fd,
                                   FILE *destination_file,
                                   int *line_count_out)
{
    char buffer[POP3_RECV_BUFFER_SIZE + 1];
    int line_count = 0;
    bool first_line_consumed = false;
    bool prev_was_newline = true;

    while (true) {
        ssize_t received = recv(socket_fd, buffer, POP3_RECV_BUFFER_SIZE, 0);
        if (received <= 0) {
            return -1;
        }
        buffer[received] = '\0';

        if (!first_line_consumed) {
            if (!response_is_ok(buffer)) {
                return -1;
            }
            first_line_consumed = true;
        }

        for (ssize_t i = 0; i < received; ++i) {
            if (destination_file != NULL) {
                fputc(buffer[i], destination_file);
            }
            if (buffer[i] == '\n') {
                ++line_count;
                prev_was_newline = true;
            } else if (buffer[i] != '\r') {
                prev_was_newline = false;
            }
        }

        /* Termination marker: "\r\n.\r\n" or ".\r\n" at the start of a line. */
        if (received >= 3 && prev_was_newline) {
            const char *tail = buffer + received - 3;
            if (tail[0] == '.' && tail[1] == '\r' && tail[2] == '\n') {
                break;
            }
        }
        if (received >= 5) {
            const char *tail = buffer + received - 5;
            if (tail[0] == '\r' && tail[1] == '\n' &&
                tail[2] == '.'  && tail[3] == '\r' && tail[4] == '\n') {
                break;
            }
        }
    }

    if (line_count_out != NULL) {
        *line_count_out = line_count;
    }
    return 0;
}

static int count_uidl_messages(int socket_fd)
{
    if (!send_command(socket_fd, "UIDL\r\n")) {
        return -1;
    }
    int line_count = 0;
    if (read_multiline_response(socket_fd, NULL, &line_count) != 0) {
        return -1;
    }
    /* Subtract +OK header line and the terminating "." line. */
    int message_count = line_count - 2;
    return (message_count < 0) ? 0 : message_count;
}

static int retrieve_message(int socket_fd, int message_index, FILE *output_file)
{
    if (!send_command(socket_fd, "RETR %d\r\n", message_index)) {
        return -1;
    }
    return read_multiline_response(socket_fd, output_file, NULL);
}

int pop3_fetch_all_attachments(int socket_fd)
{
    int message_count = count_uidl_messages(socket_fd);
    if (message_count < 0) {
        return -1;
    }
    int total_attachments = 0;
    for (int message_index = 1; message_index <= message_count; ++message_index) {
        printf("Downloading message %d/%d\n", message_index, message_count);

        char temp_path[] = "/tmp/pop3_msg_XXXXXX";
        int temp_fd = mkstemp(temp_path);
        if (temp_fd < 0) {
            return -1;
        }
        FILE *temp_file = fdopen(temp_fd, "w+b");
        if (temp_file == NULL) {
            close(temp_fd);
            unlink(temp_path);
            return -1;
        }

        if (retrieve_message(socket_fd, message_index, temp_file) != 0) {
            fclose(temp_file);
            unlink(temp_path);
            return -1;
        }

        rewind(temp_file);
        int attachments = mime_extract_attachments(temp_file);
        fclose(temp_file);
        unlink(temp_path);

        if (attachments < 0) {
            return -1;
        }
        total_attachments += attachments;
    }
    return total_attachments;
}

void pop3_quit(int socket_fd)
{
    char response[POP3_RECV_BUFFER_SIZE];
    if (!send_command(socket_fd, "QUIT\r\n")) {
        return;
    }
    (void)recv_line(socket_fd, response, sizeof(response));
}

void pop3_close(int socket_fd)
{
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}
