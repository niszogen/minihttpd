#include "lib/minihttpd.h"

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

struct server {
	int port;
	const char *public_dir;
};

static minihttpd_t *s_server = {0};

static char *get_mime_type(const char *path) {
	const char *dot = strrchr(path, '.');
	if (!dot)
		return "application/octet-stream";
	if (strcasecmp(dot, ".html") == 0)
		return "text/html";
	else if (strcasecmp(dot, ".css") == 0)
		return "text/css";
	else if (strcasecmp(dot, ".js") == 0)
		return "application/javascript";
	else if (strcasecmp(dot, ".json") == 0)
		return "application/json";
	else if (strcasecmp(dot, ".ico") == 0)
		return "image/x-icon";
	else if (strcasecmp(dot, ".png") == 0)
		return "image/png";
	else if (strcasecmp(dot, ".jxl") == 0)
		return "image/jxl";
	else if (strcasecmp(dot, ".txt") == 0)
		return "text/plain";
	else
		return "application/octet-stream";
}

static minihttpd_response_t handler(const char *path, void *user_data) {
	struct server *server = (struct server *)user_data;
	minihttpd_response_t res = {.file_fd = -1};

	if (strstr(path, "..")) {
		return res;
	}

	char filepath[256];
	if (strcmp(path, "/") == 0) {
		snprintf(filepath, sizeof(filepath), "%s/index.html", server->public_dir);
	} else {
		snprintf(filepath, sizeof(filepath), "%s%s", server->public_dir, path);
	}

	struct stat st;
	if (stat(filepath, &st) != 0 || S_ISDIR(st.st_mode)) {
		return res;
	}

	int fd = open(filepath, O_RDONLY);
	if (fd < 0) {
		return res;
	}

	res.file_fd = fd;
	res.file_len = (size_t)st.st_size;
	res.mime_type = get_mime_type(filepath);
	return res;
}

void usage(FILE *stream, char *argv0) {
	fprintf(stream, "Usage: %s [flags] /path/to/wwwroot\n", argv0);
	fprintf(stream, "OPTIONS:\n");
	fprintf(stream, "\t--help\n\t\tPrint this help to stdout and exit with 0\n");
	fprintf(stream, "\t--port <int>\n\t\tSpecify the port number (default: 8080)\n");
}

void exception_handler(int sg) {
	(void)sg;
	printf(" SIGINT detected, stopping...\n");
	if (s_server) {
		s_server->running = 0;
	}
}

int main(int argc, char **argv) {
	struct server minihttpd = {0};
	minihttpd.port = 8080;
	minihttpd.public_dir = ".";
	int user_root = false;

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];

		if (strcmp(arg, "--help") == 0) {
			usage(stdout, argv[0]);
			return 0;
		} else if (strcmp(arg, "--port") == 0) {
			if (++i >= argc) {
				fprintf(stderr, "[ERR] port needs a value\n");
				usage(stderr, argv[0]);
				return 1;
			}
			char *end;
			long p = strtol(argv[i], &end, 10);
			if (*argv[i] == '\0' || *end != '\0' || p < 1 || p > 65535) {
				fprintf(stderr, "[ERR] invalid port '%s'\n", argv[i]);
				return 1;
			}
			minihttpd.port = p;
		} else if (!user_root) {
			minihttpd.public_dir = arg;
			user_root = true;
		} else {
			fprintf(stderr, "[ERR] unexpected argument %s\n", arg);
			usage(stderr, argv[0]);
			return 1;
		}
	}

	struct stat statbuf;
	if (stat(minihttpd.public_dir, &statbuf) != 0) {
		fprintf(stderr, "Specified path is not a valid directory!\n");
		usage(stderr, argv[0]);
		return 1;
	}

	minihttpd_t *server = minihttpd_init(minihttpd.port, handler);
	if (!server) {
		fprintf(stderr, "Failed to start server\n");
		return 1;
	}

	s_server = server;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = exception_handler;
	sigaction(SIGINT, &sa, NULL);

	server->user_data = &minihttpd;
	printf("Listening on: http://localhost:%i from: %s\n", minihttpd.port, minihttpd.public_dir);
	minihttpd_run(server);

	minihttpd_stop(server);
	minihttpd_free(server);
	return 0;
}
