# imapcl — Command-line IMAP Client in C

A command-line e-mail client that downloads messages from an IMAP server and stores each one as a separate `.eml` file. It talks the IMAP4rev1 protocol (RFC 3501) directly over raw sockets, with optional TLS via OpenSSL.

> University project for *Network Applications and Network Administration (ISA)*, Brno University of Technology, Faculty of Information Technology (2024).

## Features

- Plain IMAP (port 143) or IMAPS over TLS (port 993)
- TLS certificate verification against a custom CA file or directory, with hostname checking (SNI)
- Downloads full messages or headers only (`BODY.PEEK[HEADER]`, so messages are not marked as read)
- Can download only new messages
- Selects any mailbox (default `INBOX`)
- Handles messages of any size: responses are read until the tagged status line, and message bodies are taken from IMAP literals byte-for-byte
- IPv4 and IPv6 support (`getaddrinfo`)

## Build

Requirements: GCC, Make, and the OpenSSL development headers (`libssl-dev` on Debian/Ubuntu).

```bash
make
```

## Usage

```bash
./imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a auth_file [-b MAILBOX] -o out_dir
```

| Option | Description |
|---|---|
| `server` | IMAP server hostname or IP address (required) |
| `-p port` | Server port (default: 143, or 993 with `-T`) |
| `-T` | Use TLS (IMAPS) |
| `-c certfile` | CA certificate file used to verify the server certificate |
| `-C certaddr` | Directory with trusted CA certificates (default: `/etc/ssl/certs`) |
| `-n` | Download only new messages |
| `-h` | Download only message headers |
| `-a auth_file` | File with login credentials (required) |
| `-b MAILBOX` | Mailbox to read (default: `INBOX`) |
| `-o out_dir` | Output directory; it is created if missing (required) |

The authentication file has this format:

```text
username = your_username
password = your_password
```

### Example

```bash
$ ./imapcl imap.example.com -T -a auth.txt -o ./emails
Downloaded 5 emails from INBOX

$ ./imapcl imap.example.com -T -h -n -a auth.txt -b Work -o ./headers
Downloaded 2 new email headers from Work
```

Messages are saved as `email_<UID>.eml`, `new_<UID>.eml` or `header_<UID>.eml`.

## Project structure

```text
src/
├── main.c          # Entry point: argument parsing → connect → fetch → logout
├── args_parser.c   # Command-line options (getopt)
├── config.h        # Config structure and IMAP command templates
├── imap_client.c   # TCP connection, tagged command/response handling, LOGIN/SELECT/SEARCH/FETCH/LOGOUT
├── ssl_tls.c       # TLS handshake and certificate verification (OpenSSL)
└── utils.c         # Auth file loading, UID parsing, saving messages
manual.pdf          # Original project documentation
```

## How it works

1. Connects over TCP and, with `-T`, wraps the socket in a TLS session that verifies the server certificate.
2. Reads the server greeting and sends `LOGIN`, then `SELECT` for the chosen mailbox.
3. Runs `UID SEARCH ALL` (or `UID SEARCH UNSEEN NEW` with `-n`) to get the message UIDs.
4. For each UID it sends `UID FETCH` and reads the response until the matching tagged `OK` line. It skips over literal data so that text inside a message can't be mistaken for the status line, then writes the literal to disk.
5. Logs out and closes the connection.

## Author

Daria Kinash
