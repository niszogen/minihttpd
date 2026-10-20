#ifndef HANDLER_H
#define HANDLER_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "../lib/minihttpd.h"

char *get_mime_type(const char *path);
minihttpd_response_t handler(const char *path, void *user_data);

#endif
