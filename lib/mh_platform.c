#include "mh_platform.h"
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/sendfile.h>
#endif

ssize_t mh_write_all(int sock, const void *buf, size_t len) {
	const char *p = buf;
	size_t sent = 0;

	while (sent < len) {
		ssize_t n = write(sock, p + sent, len - sent);
		if (n < 0)
			return -1;

		sent += n;
	}

	return sent;
}

#if defined(__linux__)

ssize_t mh_sendfile(int sock, const void *hdr, size_t hdr_len, int file_fd, size_t file_len) {
	if (mh_write_all(sock, hdr, hdr_len) < 0)
		return -1;

	off_t offset = 0;
	size_t remaining = file_len;

	while (remaining > 0) {
		ssize_t r = sendfile(sock, file_fd, &offset, remaining);
		if (r < 0)
			return -1;
		if (r == 0)
			break;

		remaining -= r;
	}
	return hdr_len + file_len - remaining;
}

#else

ssize_t mh_sendfile(int sock, const void *hdr, size_t hdr_len, int file_fd, size_t file_len) {
	if (mh_write_all(sock, hdr, hdr_len) < 0)
		return -1;

	char buf[8192];
	size_t remaining = file_len;

	while (remaining > 0) {
		ssize_t r = read(file_fd, buf, remaining < sizeof buf ? remaining : sizeof buf);
		if (r < 0)
			return -1;
		if (r == 0)
			break;
		if (mh_write_all(sock, buf, r) < 0)
			return -1;

		remaining -= r;
	}
	return hdr_len + file_len - remaining;
}

#endif
