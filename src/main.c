#include "pop3_client.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s <server_host> <username> <password>\n"
            "Connects to a POP3 server, downloads every message, and extracts attachments.\n",
            program_name);
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *server_host = argv[1];
    const char *username    = argv[2];
    const char *password    = argv[3];

    int socket_fd = pop3_connect(server_host, NULL);
    if (socket_fd < 0) {
        fprintf(stderr, "Cannot connect to %s\n", server_host);
        return 1;
    }

    if (!pop3_login(socket_fd, username, password)) {
        fprintf(stderr, "Authentication failed for user %s\n", username);
        pop3_close(socket_fd);
        return 1;
    }

    int attachments = pop3_fetch_all_attachments(socket_fd);
    pop3_quit(socket_fd);
    pop3_close(socket_fd);

    if (attachments < 0) {
        fprintf(stderr, "Failed to fetch messages\n");
        return 1;
    }
    printf("Extracted %d attachment(s)\n", attachments);
    return 0;
}
