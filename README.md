# GetEmailsAttachement

![C](https://img.shields.io/badge/C-11-00599C?style=flat&logo=c&logoColor=white)
![CMake](https://img.shields.io/badge/build-CMake-064F8C?style=flat&logo=cmake&logoColor=white)
![Tests](https://img.shields.io/badge/tests-GoogleTest-4CAF50?style=flat)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?style=flat&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue?style=flat)

## What is it

A small POP3 mail client written in C that connects to a mail server, downloads every message in the mailbox, parses MIME multipart bodies, and writes each attachment to disk under its original filename. No external mail library — just BSD sockets and a hand-rolled protocol.

## Why it is interesting

- **Plain BSD sockets + `getaddrinfo`** — IPv4/IPv6-aware connection setup on top of `socket(2)`, `connect(2)`, `send(2)`, `recv(2)`. No `gethostbyname`.
- **POP3 protocol from scratch** — `USER`, `PASS`, `UIDL`, `RETR`, `QUIT`, with proper handling of multi-line responses terminated by `"\r\n.\r\n"`.
- **MIME multipart parsing** — boundary detection, `filename=` extraction with path-traversal sanitization, header-line skipping.
- **In-tree Base64 decoder** — RFC 4648 implementation; no `system()` shell-out.
- **Hermetic test suite** — GoogleTest + a loopback fake POP3 server on an ephemeral port, plus RFC 4648 vectors and end-to-end MIME fixtures.
- **Modern toolchain** — CMake (≥3.14), `-Wall -Wextra -Wpedantic` clean, GoogleTest via `FetchContent`, Doxygen on the public API.

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/get_emails_attachement <server_host> <username> <password>
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

Generate API docs:

```bash
doxygen docs/Doxyfile
# open docs/build/html/index.html
```

## Project layout

```
GetEmailsAttachement/
├── CMakeLists.txt           # Build + GoogleTest via FetchContent
├── include/
│   ├── base64.h             # RFC 4648 decoder API
│   ├── mime_parser.h        # boundary / filename / attachment extraction
│   └── pop3_client.h        # connect, login, fetch, quit
├── src/
│   ├── base64.c
│   ├── main.c               # thin entry point: arg parsing + orchestration
│   ├── mime_parser.c
│   └── pop3_client.c
├── tests/
│   ├── test_base64.cpp      # RFC vectors + edge cases
│   ├── test_mime_parser.cpp # header parsing + end-to-end attachment extraction
│   └── test_pop3_client.cpp # loopback fake-server protocol tests
├── docs/
│   └── Doxyfile             # public-API documentation
└── README.md
```
