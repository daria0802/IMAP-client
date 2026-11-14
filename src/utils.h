/// IMAP client
/// @author: Daria Kinash(xkinas00)

#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include <errno.h>
#include <sys/stat.h>

int load_auth(const char *auth_file, Config *config);
int create_output_dir(const char *path);
int get_uids(const char *response, int **uids);
int save_email_content(const char *response, const char *filename);

#endif