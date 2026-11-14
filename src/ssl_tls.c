/// IMAP client
/// @author: Daria Kinash(xkinas00)

#include "ssl_tls.h"

/**
 * Establishes a secure SSL/TLS session on an already connected socket.
 * The server certificate is verified against the CA file (-c) and/or
 * the CA directory (-C), and its name must match the requested server.
 *
 * @param config  Pointer to a Config structure.
 * @param sockfd  Connected TCP socket.
 * @param ssl_ctx Pointer to an SSL_CTX pointer to store the created SSL context.
 * @param ssl Pointer to an SSL pointer to store the SSL structure for the connection.
 *
 * @return 0 on success, -1 on error.
 */
int ssl_tls_connect(Config *config, int sockfd, SSL_CTX **ssl_ctx, SSL **ssl) {
    *ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (*ssl_ctx == NULL){
        fprintf(stderr, "Error creating SSL context\n");
        ERR_print_errors_fp(stderr);
        return -1;
    }

    SSL_CTX_set_verify(*ssl_ctx, SSL_VERIFY_PEER, NULL);

    if (SSL_CTX_load_verify_locations(*ssl_ctx, config->cert_file, config->cert_dir) <= 0) {
        fprintf(stderr, "Error loading certificates (file: %s, directory: %s)\n",
                config->cert_file ? config->cert_file : "-",
                config->cert_dir ? config->cert_dir : "-");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(*ssl_ctx);
        return -1;
    }

    *ssl = SSL_new(*ssl_ctx);
    if (*ssl == NULL) {
        fprintf(stderr, "Error creating SSL structure\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(*ssl_ctx);
        return -1;
    }

    // SNI + hostname verification
    SSL_set_tlsext_host_name(*ssl, config->server);
    SSL_set1_host(*ssl, config->server);

    SSL_set_fd(*ssl, sockfd);

    if (SSL_connect(*ssl) <= 0) {
        fprintf(stderr, "Error during SSL/TLS handshake\n");
        long verify_result = SSL_get_verify_result(*ssl);
        if (verify_result != X509_V_OK) {
            fprintf(stderr, "Certificate verification failed: %s\n",
                    X509_verify_cert_error_string(verify_result));
        }
        ERR_print_errors_fp(stderr);
        SSL_free(*ssl);
        SSL_CTX_free(*ssl_ctx);
        return -1;
    }

    return 0;
}
