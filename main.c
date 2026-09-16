#include "lib/minihttpd.h"
#include <signal.h>
#define FLAG_IMPLEMENTATION
#include "thirdparty/flag.h"

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

	if (strstr(path, "..")) {
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}

	char filepath[256];
	if (strcmp(path, "/") == 0) {
		snprintf(filepath, sizeof(filepath), "%s/index.html", server->public_dir);
	} else {
		snprintf(filepath, sizeof(filepath), "%s%s", server->public_dir, path);
	}

	FILE *f = fopen(filepath, "rb");
	if (!f) {
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}
	long size = ftell(f);
	if (size < 0) {
		fclose(f);
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}
	rewind(f);

	char *buffer = malloc((size_t)size);
	if (!buffer) {
		fclose(f);
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}

	size_t read_bytes = fread(buffer, 1, (size_t)size, f);
	fclose(f);

	if (read_bytes != (size_t)size) {
		free(buffer);
		return (minihttpd_response_t){NULL, 0, NULL, 0};
	}

	return (minihttpd_response_t){buffer, (size_t)size, get_mime_type(filepath), 1};
}

void usage(FILE *stream, char *argv0) {
	fprintf(stream, "Usage: %s [flags] /path/to/wwwroot\n", argv0);
	fprintf(stream, "OPTIONS:\n");
	flag_print_options(stream);
}

void exception_handler(int sg) {
	(void)sg;
	printf("SIGINT detected, stopping...\n");
	if (s_server) {
		s_server->running = 0;
	}
}

int main(int argc, char **argv) {
	struct server minihttpd = {0};
	char *argv0 = argv[0];

	bool *help = flag_bool("-help", false, "Print this help to stdout and exit with 0");
	size_t *port = flag_size("-port", 8080, "Specify the port number");

	if (!flag_parse(argc, argv)) {
		usage(stderr, argv0);
		flag_print_error(stderr);
		exit(1);
	}

	argc = flag_rest_argc();
	argv = flag_rest_argv();

	if (argc > 0) {
		minihttpd.public_dir = argv[0];
	} else
		minihttpd.public_dir = ".";

	struct stat statbuf;
	if (stat(minihttpd.public_dir, &statbuf) != 0) {
		fprintf(stderr, "Specified path is not a valid directory!\n");
		usage(stderr, argv0);
		return 1;
	}

	if (*help) {
		usage(stdout, argv0);
		exit(0);
	}

	minihttpd_t *server = minihttpd_init(*port, handler);
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
	printf("Listening on: http://localhost:%i from: %s\n", (int)*port, minihttpd.public_dir);
	minihttpd_run(server);

	minihttpd_stop(server);
	minihttpd_free(server);
	return 0;
}
