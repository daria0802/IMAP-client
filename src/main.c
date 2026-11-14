/// IMAP client
/// @author: Daria Kinash(xkinas00)

#include "args_parser.h"
#include "imap_client.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    Config config;

    // Parse command-line arguments
    if (parse_args(argc, argv, &config) == -1) {
        return EXIT_FAILURE;
    }

    // Read authentication file
    if (load_auth(config.auth_file, &config) == -1) {
        fprintf(stderr, "Error reading authentication file\n");
        return EXIT_FAILURE;
    }

    // Connect to the IMAP server
    int sockfd = imap_connect(&config);
    if (sockfd == -1) {
        fprintf(stderr, "Error connecting to the IMAP server\n");
        free(config.username);
        free(config.password);
        return EXIT_FAILURE;
    }

    // Fetch and store emails
    int num_emails = imap_fetch_emails(sockfd, &config);
    if (num_emails == -1) {
        fprintf(stderr, "Error fetching emails\n");
        free(config.username);
        free(config.password);
        return EXIT_FAILURE;
    }

    // Output the number of downloaded emails
    if(config.new_only && config.headers_only){
        printf("Downloaded %d new email headers from %s\n", num_emails, config.mailbox);
    } else if(config.new_only){
        printf("Downloaded %d new emails from %s\n", num_emails, config.mailbox);
    } else if(config.headers_only){
        printf("Downloaded %d email headers from %s\n", num_emails, config.mailbox);
    } else {
        printf("Downloaded %d emails from %s\n", num_emails, config.mailbox);
    }

    free(config.username);
    free(config.password);

    return EXIT_SUCCESS;
}
