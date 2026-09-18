#ifndef MH_PLATFORM_H
#define MH_PLATFORM_H

#include <stddef.h>
#include <sys/types.h>

ssize_t mh_write_all(int sock, const void *buf, size_t len);
ssize_t mh_sendfile(int sock, const void *hdr, size_t hdr_len, int file_fd, size_t file_len);

#endif
