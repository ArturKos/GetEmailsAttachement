# GetEmailsAttachement

![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?style=flat&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue?style=flat)

POP3 email client that connects to a mail server, retrieves all messages, parses MIME structure, and automatically extracts file attachments. Built on raw BSD sockets with a hand-rolled POP3 protocol and MIME parser.

## Features

- **Raw socket POP3 implementation** -- communicates with POP3 servers on port 110 using POSIX sockets (`socket`, `connect`, `send`, `recv`) with no external mail library
- **Full mailbox retrieval** -- enumerates messages via `UIDL`, then downloads each with `RETR` and writes raw content to temporary files for processing
- **MIME boundary parsing** -- detects `boundary=` headers in multipart messages to correctly delimit individual MIME parts
- **Automatic attachment extraction** -- locates `filename=` headers within MIME parts, extracts the Base64-encoded body, and decodes it via an external `b64decode` utility
- **Multi-attachment support** -- iterates through all MIME boundaries in a message, extracting every attachment found
- **Keepalive via NOP** -- sends `NOP` commands to maintain the server connection during long operations

## Dependencies

| Dependency | Purpose |
|---|---|
| GCC | C compiler |
| POSIX sockets (`arpa/inet.h`, `netinet/in.h`) | Network communication |
| `netdb.h` | DNS hostname resolution |
| `b64decode` | External Base64 decoding utility (expected in `PATH` or working directory) |

## Build and Run

```bash
gcc mymime.c -o mymime
./mymime <server_address> <username> <password>
```

Example:

```bash
./mymime pop3.example.com john.doe secretpass
```

Extracted attachments are saved to the current working directory with their original filenames.

## Project Structure

```
GetEmailsAttachement/
  mymime.c       # Main entry point -- argument parsing, connection orchestration
  mymime.h       # POP3 protocol, MIME parsing, and attachment extraction functions
  build           # Build helper script
```

## How It Works

1. A TCP socket connects to port 110 of the specified POP3 server.
2. The client authenticates with `USER` / `PASS` commands.
3. `UIDL` retrieves the unique ID listing to determine the message count.
4. Each message is downloaded via `RETR` and saved to a temporary file.
5. The MIME parser scans for `boundary=` to identify multipart sections.
6. Within each section, `filename=` headers identify attachments.
7. Base64-encoded attachment bodies are written to a temporary file and decoded using `b64decode`.
8. The decoded file is saved under its original filename; temporary files are cleaned up.
