#ifdef __NetBSD__
#ifndef _NETBSD_SOURCE
#define _NETBSD_SOURCE
#endif
#elif defined(__OpenBSD__)
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#elif defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include "mh_platform.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/sendfile.h>
#endif

int mh_accept(int sock, struct sockaddr *addr, socklen_t *addrlen) {
#if defined(__NetBSD__)
	return paccept(sock, addr, addrlen, NULL, SOCK_CLOEXEC);
#elif defined(__OpenBSD__) || defined(__linux__) || defined(__FreeBSD__)
	return accept4(sock, addr, addrlen, SOCK_CLOEXEC);
#else
	int fd = accept(sock, addr, addrlen);
	if (fd >= 0) {
		fcntl(fd, F_SETFD, FD_CLOEXEC);
	}
	return fd;
#endif
}

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

#ifdef __linux__

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
