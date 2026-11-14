/// IMAP client
/// @author: Daria Kinash(xkinas00)

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Commands without the tag; execute_command() prefixes them with "A001 ", "A002 ", ...
#define IMAP_LOGIN "LOGIN \"%s\" \"%s\"\r\n"
#define IMAP_SELECT "SELECT \"%s\"\r\n"
#define IMAP_COUNT "UID SEARCH %s\r\n"
#define IMAP_FETCH_HEADERS "UID FETCH %d (BODY.PEEK[HEADER])\r\n"
#define IMAP_FETCH "UID FETCH %d (RFC822)\r\n"
#define IMAP_LOGOUT "LOGOUT\r\n"

#define IMAP_PORT 143
#define IMAPS_PORT 993
#define BUFFER_SIZE 1024
#define TAG_SIZE 16

typedef struct {
    char *server;        // Server name or IP
    int port;            // Port number (0 = choose default based on encryption)
    bool encryption;     // Use TLS (imaps)
    char *cert_file;     // Certificate file for SSL/TLS
    char *cert_dir;      // Directory for SSL certificates
    bool new_only;       // Fetch only new messages
    bool headers_only;   // Fetch headers
    char *auth_file;     // Authentication file
    char *mailbox;       // Mailbox name
    char *output_dir;    // Output directory for saving emails
    char *username;      // Username for authentication
    char *password;      // Password for authentication
} Config;

#endif
