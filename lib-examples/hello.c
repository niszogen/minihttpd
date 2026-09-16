#include "../minihttpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static minihttpd_response_t handler(const char *path, void *user_data) {
	(void)user_data;

	if (strcmp(path, "/") == 0) {
		static const char *body = "Hello from libminihttpd!\n";
		return (minihttpd_response_t){body, strlen(body), "text/plain", 0};
	}

	if (strcmp(path, "/hi.json") == 0) {
		static const char *body = "{\"message\":\"hi :D\"}\n";
		return (minihttpd_response_t){body, strlen(body), "application/json", 0};
	}

	if (strcmp(path, "/date.json") == 0) {
		time_t now = time(NULL);
		struct tm tm = *localtime(&now);

		static char body[128];
		snprintf(body, sizeof(body),
				 "{\"now\":\"%d-%02d-%02d %02d:%02d:%02d\"}\n",
				 tm.tm_year + 1900,
				 tm.tm_mon + 1,
				 tm.tm_mday,
				 tm.tm_hour,
				 tm.tm_min,
				 tm.tm_sec);
		return (minihttpd_response_t){body, strlen(body), "application/json", 0};
	}

	return (minihttpd_response_t){NULL, 0, NULL, 0}; // -> 404
}

int main(int argc, char **argv) {
	int port = 8080;
	if (argc == 3 && strcmp(argv[1], "--port") == 0)
		port = atoi(argv[2]);

	minihttpd_t *server = minihttpd_init(port, handler);
	if (!server) {
		fprintf(stderr, "Failed to start server\n");
		return 1;
	}

	printf("Listening on http://localhost:%i\n", port);
	minihttpd_run(server);

	minihttpd_free(server);
	return 0;
}
