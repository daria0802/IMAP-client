/// IMAP client
/// @author: Daria Kinash(xkinas00)

#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H

#include "config.h"
#include "ssl_tls.h"
#include "utils.h"
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>


int imap_connect(Config *config);
int imap_fetch_emails(int sockfd, Config *config);
int imap_login(int sockfd, Config *config);
int imap_logout(int sockfd, Config *config);
int imap_select_mailbox(int sockfd, Config *config);
int imap_count_emails(int sockfd, Config *config, int **uids, int *uid_count);
int imap_fetch_messages(int sockfd, Config *config);

#endif
