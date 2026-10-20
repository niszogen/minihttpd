#include "main.h"

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

#ifdef __OpenBSD__
#include <unistd.h>
#endif

#include "handler.h"
#include "../lib/minihttpd.h"

static minihttpd_t *s_server = {0};

void usage(FILE *stream, char *argv0) {
	fprintf(stream, "Usage: %s [flags] /path/to/wwwroot\n", argv0);
	fprintf(stream, "OPTIONS:\n");
	fprintf(stream, "\t--help\n\t\tPrint this help to stdout and exit with 0\n");
	fprintf(stream, "\t--port <int>\n\t\tSpecify the port number (default: 8080)\n");
	fprintf(stream, "\t--addr <string>\n\t\tSpecify the listen address (default: 0.0.0.0)\n");
}

static void exception_handler(int sg) {
	(void)sg;
	printf(" SIGINT detected, stopping...\n");
	if (s_server) {
		s_server->running = 0;
	}
}

int main(const int argc, char **argv) {
	struct server minihttpd = {0};
	minihttpd.addr = NULL;
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
			const long p = strtol(argv[i], &end, 10);
			if (*argv[i] == '\0' || *end != '\0' || p < 1 || p > 65535) {
				fprintf(stderr, "[ERR] invalid port '%s'\n", argv[i]);
				return 1;
			}
			minihttpd.port = p;
		} else if (strcmp(arg, "--addr") == 0) {
			if (++i >= argc) {
				fprintf(stderr, "[ERR] addr needs a value\n");
				usage(stderr, argv[0]);
				return 1;
			}
			minihttpd.addr = argv[i];
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

	minihttpd_t *server = minihttpd_init(minihttpd.addr, minihttpd.port, handler);
	if (!server) {
		fprintf(stderr, "Failed to start server\n");
		return 1;
	}

	s_server = server;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = exception_handler;
	sigaction(SIGINT, &sa, NULL);

#ifdef __OpenBSD__
	if (unveil(minihttpd.public_dir, "r") == -1) {
		fprintf(stderr, "[ERR] Cannot call unveil");
		return 1;
	}

	if (pledge("stdio inet rpath", 0) == -1) {
		fprintf(stderr, "[ERR] Cannot call pledge");
		return 1;
	}
#endif

	server->user_data = &minihttpd;
	printf("Listening on: http://%s:%i from: %s\n",
		   minihttpd.addr ? minihttpd.addr : "localhost",
		   minihttpd.port, minihttpd.public_dir);
	minihttpd_run(server);

	minihttpd_stop(server);
	minihttpd_free(server);
	return 0;
}
