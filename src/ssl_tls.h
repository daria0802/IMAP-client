/// IMAP client
/// @author: Daria Kinash(xkinas00)

#ifndef SSL_TLS_H
#define SSL_TLS_H

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

int ssl_tls_connect(Config *config, int sockfd, SSL_CTX **ssl_ctx, SSL **ssl);

#endif
