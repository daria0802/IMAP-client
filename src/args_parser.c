/// IMAP client
/// @author: Daria Kinash(xkinas00)

#include "args_parser.h"

static void print_usage(const char *prog) {
    fprintf(stderr,
            "Usage: %s server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] "
            "-a auth_file [-b MAILBOX] -o out_dir\n", prog);
}

int parse_args(int argc, char *argv[], Config *config) {
    int opt;

    memset(config, 0, sizeof(*config));
    config->mailbox = "INBOX";
    config->cert_dir = "/etc/ssl/certs";

    while ((opt = getopt(argc, argv, "p:Tc:C:nha:b:o:")) != -1) {
        switch (opt) {
            case 'p': {
                char *end;
                long port = strtol(optarg, &end, 10);
                if (*end != '\0' || port <= 0 || port > 65535) {
                    fprintf(stderr, "Invalid port: %s\n", optarg);
                    return -1;
                }
                config->port = (int)port;
                break;
            }
            case 'T':
                config->encryption = true;
                break;
            case 'c':
                config->cert_file = optarg;
                break;
            case 'C':
                config->cert_dir = optarg;
                break;
            case 'n':
                config->new_only = true;
                break;
            case 'h':
                config->headers_only = true;
                break;
            case 'a':
                config->auth_file = optarg;
                break;
            case 'b':
                config->mailbox = optarg;
                break;
            case 'o':
                config->output_dir = optarg;
                break;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }

    if (optind < argc) {
        config->server = argv[optind];
    }

    // Default port depends on -T, so it is chosen only after all options are read
    if (config->port == 0) {
        config->port = config->encryption ? IMAPS_PORT : IMAP_PORT;
    }

    if (config->server == NULL || config->auth_file == NULL || config->output_dir == NULL) {
        fprintf(stderr, "Mandatory parameters are missing\n");
        print_usage(argv[0]);
        return -1;
    }
    return 0;
}
