/// IMAP client
/// @author: Daria Kinash(xkinas00)

#ifndef ARGS_PARSER_H
#define ARGS_PARSER_H

#include "config.h"
#include <getopt.h>

int parse_args(int argc, char *argv[], Config *config);

#endif