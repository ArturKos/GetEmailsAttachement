#ifndef POP3_CLIENT_H
#define POP3_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default TCP port for plain POP3 (RFC 1939). */
#define POP3_DEFAULT_PORT "110"

/** Maximum bytes read in a single recv() call from the POP3 server. */
#define POP3_RECV_BUFFER_SIZE 4096

/**
 * @brief Open a TCP connection to a POP3 server.
 *
 * Resolves the hostname via @c getaddrinfo and connects to the first
 * address that accepts the connection.
 *
 * @param server_host  Hostname or IPv4/IPv6 address of the POP3 server.
 * @param server_port  Port as a string (e.g. "110"); if NULL, defaults
 *                     to @ref POP3_DEFAULT_PORT.
 * @return Connected socket file descriptor on success, -1 on failure.
 */
int pop3_connect(const char *server_host, const char *server_port);

/**
 * @brief Authenticate with USER / PASS commands.
 *
 * @param socket_fd Connected socket from @ref pop3_connect.
 * @param username  POP3 username, NUL-terminated.
 * @param password  POP3 password, NUL-terminated.
 * @return true on +OK from both commands, false otherwise.
 */
bool pop3_login(int socket_fd, const char *username, const char *password);

/**
 * @brief Download every message in the mailbox and extract attachments.
 *
 * Issues UIDL to count messages, then RETR for each, streaming each
 * message into a temporary file passed to the MIME parser.
 *
 * @param socket_fd Authenticated POP3 socket.
 * @return Total number of attachments extracted, or -1 on protocol error.
 */
int pop3_fetch_all_attachments(int socket_fd);

/**
 * @brief Send the QUIT command.
 *
 * @param socket_fd Authenticated POP3 socket.
 */
void pop3_quit(int socket_fd);

/**
 * @brief Close the underlying socket.
 *
 * Always pair with @ref pop3_connect.
 *
 * @param socket_fd Socket to close.
 */
void pop3_close(int socket_fd);

#ifdef __cplusplus
}
#endif

#endif
