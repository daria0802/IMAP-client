/// IMAP client
/// @author: Daria Kinash(xkinas00)

#include "imap_client.h"
#include <stdarg.h>

// Global variables for SSL/TLS context
SSL_CTX *ssl_ctx = NULL;
SSL *ssl = NULL;
int tag_counter = 1;

/**
 * Generates the next command tag (A001, A002, ...).
 *
 * @param tag Buffer of at least TAG_SIZE bytes.
 */
static void next_tag(char *tag) {
    snprintf(tag, TAG_SIZE, "A%03d", tag_counter++);
}

/**
 * Sends the whole command over a plain or encrypted connection.
 *
 * @return 0 on success, -1 on failure.
 */
static int send_command(int sockfd, Config *config, const char *command) {
    size_t total = strlen(command);
    size_t sent = 0;

    while (sent < total) {
        int ret;
        if (config->encryption) {
            ret = SSL_write(ssl, command + sent, (int)(total - sent));
            if (ret <= 0) {
                fprintf(stderr, "SSL_write error: %d\n", SSL_get_error(ssl, ret));
                return -1;
            }
        } else {
            ret = send(sockfd, command + sent, total - sent, 0);
            if (ret == -1) {
                perror("Error sending command");
                return -1;
            }
        }
        sent += ret;
    }
    return 0;
}

/**
 * Reads one chunk of data from a plain or encrypted connection.
 *
 * @return Number of bytes read, or <= 0 on error / closed connection.
 */
static int recv_chunk(int sockfd, Config *config, char *buf, int len) {
    if (config->encryption) {
        return SSL_read(ssl, buf, len);
    }
    return recv(sockfd, buf, len, 0);
}

/**
 * Finds the start of the tagged status line (e.g. "A001 OK ...") in a response.
 * Literal data ("{123}\r\n" followed by 123 bytes, e.g. a message body) is
 * skipped, so a line inside an email can never be mistaken for the status line.
 *
 * @return Pointer to the tagged line, or NULL if it has not been received yet.
 */
static const char *find_tagged_line(const char *response, const char *tag) {
    size_t tag_len = strlen(tag);
    size_t total = strlen(response);
    const char *line = response;

    while (*line) {
        const char *eol = strstr(line, "\r\n");
        if (!eol) {
            return NULL;    // incomplete line, more data needed
        }
        if (strncmp(line, tag, tag_len) == 0 && line[tag_len] == ' ') {
            return line;
        }

        const char *next = eol + 2;

        // Line ending with "{N}" announces N bytes of literal data
        if (eol > line && eol[-1] == '}') {
            const char *brace = eol - 1;
            while (brace > line && *brace != '{') {
                brace--;
            }
            if (*brace == '{') {
                size_t literal_len = strtoul(brace + 1, NULL, 10);
                if ((size_t)(next - response) + literal_len > total) {
                    return NULL;    // literal not fully received yet
                }
                next += literal_len;
            }
        }
        line = next;
    }
    return NULL;
}

/**
 * Reads a complete server response. With a tag, it reads until the tagged
 * status line arrives (so large FETCH responses are not truncated). Without
 * a tag, it reads a single line (used for the server greeting).
 *
 * @return Heap-allocated, NUL-terminated response (caller frees), or NULL on error.
 */
static char *read_response(int sockfd, Config *config, const char *tag) {
    size_t capacity = BUFFER_SIZE;
    size_t length = 0;
    char *response = malloc(capacity);
    if (!response) {
        perror("Failed to allocate memory for response");
        return NULL;
    }
    response[0] = '\0';

    while (1) {
        if (tag ? find_tagged_line(response, tag) != NULL : strstr(response, "\r\n") != NULL) {
            return response;
        }

        if (capacity - length < BUFFER_SIZE) {
            capacity *= 2;
            char *tmp = realloc(response, capacity);
            if (!tmp) {
                perror("Failed to reallocate memory for response");
                free(response);
                return NULL;
            }
            response = tmp;
        }

        int bytes_received = recv_chunk(sockfd, config, response + length, (int)(capacity - length - 1));
        if (bytes_received <= 0) {
            fprintf(stderr, "Error reading server response (connection closed?)\n");
            free(response);
            return NULL;
        }
        length += bytes_received;
        response[length] = '\0';
    }
}

/**
 * Sends a tagged command and waits for its complete response.
 *
 * @param tag Buffer (TAG_SIZE) that receives the tag used for this command.
 * @param format Command format without the tag (see config.h).
 * @return Heap-allocated response (caller frees), or NULL on error.
 */
static char *execute_command(int sockfd, Config *config, char *tag, const char *format, ...) {
    char command[BUFFER_SIZE];
    va_list args;

    next_tag(tag);
    int prefix_len = snprintf(command, sizeof(command), "%s ", tag);

    va_start(args, format);
    vsnprintf(command + prefix_len, sizeof(command) - prefix_len, format, args);
    va_end(args);

    if (send_command(sockfd, config, command) == -1) {
        return NULL;
    }
    return read_response(sockfd, config, tag);
}

/**
 * Checks whether the tagged status line of a response is OK.
 */
static int is_tagged_ok(const char *response, const char *tag) {
    const char *line = find_tagged_line(response, tag);
    return line && strncmp(line + strlen(tag), " OK", 3) == 0;
}

/**
 * Opens a TCP connection to the server (IPv4 or IPv6).
 *
 * @return Socket descriptor on success, -1 on failure.
 */
static int tcp_connect(Config *config) {
    struct addrinfo hints, *result, *rp;
    char port[8];
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(port, sizeof(port), "%d", config->port);

    int err = getaddrinfo(config->server, port, &hints, &result);
    if (err != 0) {
        fprintf(stderr, "Error resolving hostname %s: %s\n", config->server, gai_strerror(err));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(result);

    if (sockfd == -1) {
        fprintf(stderr, "Error connecting to %s:%d\n", config->server, config->port);
    }
    return sockfd;
}

/**
 * Connects to the IMAP server.
 *
 * @param config Pointer to the Config structure containing connection details (server, port, encryption).
 * @return Socket descriptor on success, -1 on failure.
 */
int imap_connect(Config *config){
    int sockfd = tcp_connect(config);
    if (sockfd == -1) {
        return -1;
    }

    if (config->encryption && ssl_tls_connect(config, sockfd, &ssl_ctx, &ssl) == -1) {
        fprintf(stderr, "Error establishing SSL/TLS connection\n");
        close(sockfd);
        return -1;
    }

    // Server greeting: "* OK ..."
    char *greeting = read_response(sockfd, config, NULL);
    if (!greeting) {
        return -1;
    }
    if (strncmp(greeting, "* OK", 4) != 0 && strncmp(greeting, "* PREAUTH", 9) != 0) {
        fprintf(stderr, "Unexpected server greeting: %s", greeting);
        free(greeting);
        return -1;
    }
    free(greeting);

    return sockfd;
}

/**
 * Fetches emails from the IMAP server.
 *
 * @param sockfd Socket descriptor for the connection.
 * @param config Pointer to the configuration structure.
 * @return Number of emails fetched on success, -1 on failure.
 */
int imap_fetch_emails(int sockfd, Config *config){
    if(imap_login(sockfd, config) == -1){
        fprintf(stderr, "Error logging in to the IMAP server\n");
        return -1;
    }

    if(imap_select_mailbox(sockfd, config) == -1){
        fprintf(stderr, "Error selecting mailbox: %s\n", config->mailbox);
        return -1;
    }

    int num_emails = imap_fetch_messages(sockfd, config);
    if (num_emails == -1) {
        fprintf(stderr, "Error fetching emails\n");
        return -1;
    }

    if(imap_logout(sockfd, config) == -1){
        fprintf(stderr, "Error logging out from the IMAP server\n");
        return -1;
    }

    return num_emails;
}

/**
 * Logs in to the IMAP server.
 *
 * @param sockfd Socket descriptor.
 * @param config Pointer to the Config structure containing username and password.
 * @return 0 on success, -1 on failure.
 */
int imap_login(int sockfd, Config *config) {
    char tag[TAG_SIZE];
    char *response = execute_command(sockfd, config, tag, IMAP_LOGIN, config->username, config->password);
    if (!response) {
        return -1;
    }

    int ok = is_tagged_ok(response, tag);
    if (!ok) {
        fprintf(stderr, "Login failed: %s", find_tagged_line(response, tag));
    }
    free(response);
    return ok ? 0 : -1;
}

/**
 * Logs out from the IMAP server and cleans up resources.
 *
 * @param sockfd Socket descriptor.
 * @param config Pointer to the Config structure.
 * @return 0 on success, -1 on failure.
 */
int imap_logout(int sockfd, Config *config) {
    char tag[TAG_SIZE];
    char *response = execute_command(sockfd, config, tag, IMAP_LOGOUT);
    int ok = response && strstr(response, "BYE") != NULL;
    free(response);

    if (!ok) {
        fprintf(stderr, "Error: Logout failed or not acknowledged by server\n");
    }

    if (config->encryption) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
    }

    close(sockfd);

    return ok ? 0 : -1;
}

/**
 * Selects a mailbox on the IMAP server.
 *
 * @param sockfd Socket descriptor.
 * @param config Pointer to the Config structure containing the mailbox name.
 * @return 0 on success, -1 on failure.
 */
int imap_select_mailbox(int sockfd, Config* config) {
    char tag[TAG_SIZE];
    char *response = execute_command(sockfd, config, tag, IMAP_SELECT, config->mailbox);
    if (!response) {
        return -1;
    }

    int ok = is_tagged_ok(response, tag);
    if (!ok) {
        fprintf(stderr, "Error selecting mailbox: %s", find_tagged_line(response, tag));
    }
    free(response);
    return ok ? 0 : -1;
}

/**
 * Counts emails in the selected mailbox based on the specified criteria.
 *
 * @param sockfd Socket descriptor for the connection.
 * @param config Pointer to the configuration structure.
 * @param uids Pointer to an integer array to store the UIDs of the emails.
 * @param uid_count Pointer to an integer to store the number of UIDs found.
 * @return 0 on success, -1 on failure.
 */
int imap_count_emails(int sockfd, Config *config, int **uids, int *uid_count) {
    char tag[TAG_SIZE];
    char *response = execute_command(sockfd, config, tag, IMAP_COUNT, (config->new_only ? "UNSEEN NEW" : "ALL"));
    if (!response) {
        return -1;
    }
    if (!is_tagged_ok(response, tag)) {
        fprintf(stderr, "SEARCH failed: %s", find_tagged_line(response, tag));
        free(response);
        return -1;
    }

    *uid_count = get_uids(response, uids);
    free(response);

    return *uid_count == -1 ? -1 : 0;
}

/**
 * Fetches messages (full or headers only, all or new only, depending on the
 * configuration) and saves each one to its own file in the output directory.
 *
 * @param sockfd Socket descriptor for the connection.
 * @param config Pointer to the configuration structure.
 * @return Number of messages fetched on success, -1 on failure.
 */
int imap_fetch_messages(int sockfd, Config *config) {
    int *uids;
    int uid_count;

    if (imap_count_emails(sockfd, config, &uids, &uid_count) == -1) {
        fprintf(stderr, "Error searching emails\n");
        return -1;
    }

    if (create_output_dir(config->output_dir) == -1) {
        free(uids);
        return -1;
    }

    const char *fetch_format = config->headers_only ? IMAP_FETCH_HEADERS : IMAP_FETCH;
    const char *prefix = config->headers_only ? "header" : (config->new_only ? "new" : "email");
    int email_counter = 0;

    for (int i = 0; i < uid_count; i++) {
        char tag[TAG_SIZE];
        char *response = execute_command(sockfd, config, tag, fetch_format, uids[i]);
        if (!response) {
            free(uids);
            return -1;
        }
        if (!is_tagged_ok(response, tag)) {
            fprintf(stderr, "FETCH of UID %d failed\n", uids[i]);
            free(response);
            continue;
        }

        char filename[BUFFER_SIZE];
        snprintf(filename, sizeof(filename), "%s/%s_%d.eml", config->output_dir, prefix, uids[i]);

        if (save_email_content(response, filename) == -1) {
            fprintf(stderr, "Error saving email content\n");
            free(response);
            free(uids);
            return -1;
        }
        free(response);
        email_counter++;
    }

    free(uids);

    return email_counter;
}
