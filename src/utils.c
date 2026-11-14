/// IMAP client
/// @author: Daria Kinash(xkinas00)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/**
 * Loads authentication details (username and password) from a file.
 *
 * @param filename Path to the authentication file.
 * @param config Pointer to the Config structure to store the loaded username and password.
 * @return 0 on success, -1 on failure.
 */
int load_auth(const char *filename, Config *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening authentication file");
        return -1;
    }

    char line[BUFFER_SIZE];
    char username[BUFFER_SIZE];
    char password[BUFFER_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "username = %s\n", username) == 1) {
            config->username = strdup(username);
        } else if (sscanf(line, "password = %s\n", password) == 1) {
            config->password = strdup(password);
        }
    }
    
    fclose(file);

    if (!config->username || !config->password) {
        fprintf(stderr, "Authentication file must contain 'username = ...' and 'password = ...'\n");
        free(config->username);
        free(config->password);
        return -1;
    }
    return 0;
}

/**
 * Creates an output directory if it does not already exist.
 *
 * @param path Path to the directory to create.
 * @return 0 on success, -1 on failure.
 */
int create_output_dir(const char *path) {
    struct stat sb;

    if (!(stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))) {
        if (mkdir(path, 0755) == -1) {
            if (errno != EEXIST) {
                perror("Error creating output directory");
                return -1;
            }
        }
    }
    return 0;
}

/**
 * Checks if a character is a whitespace character.
 *
 * @param c The character to check.
 * @return 1 if the character is a whitespace character, 0 otherwise.
 */
int is_space(char c){
    return( c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f');
}

/**
 * Checks if a character is a digit.
 *
 * @param c The character to check.
 * @return 1 if the character is a digit, 0 otherwise.
 */
int is_digit(char c){
    return (c <= '9' && c >= '0' );
}

/**
 * Extracts UIDs from a SEARCH response.
 *
 * @param response The response from the IMAP server.
 * @param uids Pointer to an array to store the UIDs.
 * @return The number of UIDs extracted on success, -1 on failure.
 */
int get_uids(const char *response, int **uids) {
    const char *start = strstr(response, "SEARCH");
    if (!start) {
        fprintf(stderr, "SEARCH keyword not found in response.\n");
        return -1;
    }
    start += 6; // Skip SEARCH

    while (*start && is_space(*start)) start++;

    int capacity = 10;
    int uid_count = 0;
    *uids = (int *)malloc(capacity * sizeof(int));
    if (!*uids) {
        perror("Failed to allocate memory for UIDs");
        return -1;
    }

    while (*start && is_digit(*start)) {
        if (uid_count == capacity) {
            capacity *= 2;
            int *tmp = (int *)realloc(*uids, capacity * sizeof(int));
            if (!tmp) {
                perror("Failed to reallocate memory for UIDs");
                free(*uids);
                return -1;
            }
            *uids = tmp;
        }
        (*uids)[uid_count++] = strtol(start, (char **)&start, 10);
        while (*start && is_space(*start)) start++;
    }

    return uid_count;
}

/**
 * Saves the email content to a file.
 * The message is taken from the IMAP literal in the FETCH response
 * ("... {N}\r\n" followed by exactly N bytes), so it is written byte-for-byte.
 *
 * @param response The complete FETCH response from the IMAP server.
 * @param filename The name of the file to save the email content.
 * @return 0 on success, -1 on failure.
 */
int save_email_content(const char *response, const char *filename){
    const char *brace = strchr(response, '{');
    const char *content_start;
    size_t content_len;

    if (brace && strstr(brace, "}\r\n")) {
        content_len = strtoul(brace + 1, NULL, 10);
        content_start = strstr(brace, "}\r\n") + 3;
    } else {
        // Fallback for servers that return the message as a quoted string
        content_start = strstr(response, "\r\n");
        content_start = content_start ? content_start + 2 : response;
        const char *content_end = strstr(content_start, "\r\n)");
        content_len = content_end ? (size_t)(content_end - content_start) : strlen(content_start);
    }

    FILE *file = fopen(filename, "wb");
    if(!file){
        perror("Error opening output file");
        return -1;
    }

    if (fwrite(content_start, 1, content_len, file) != content_len) {
        perror("Error writing output file");
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}
