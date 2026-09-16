#ifndef MINIHTTPD_H
#define MINIHTTPD_H

#include <stddef.h>

typedef struct {
	const char *content;
	size_t content_len;
	const char *mime_type;
	int owns_content;
} minihttpd_response_t;

typedef minihttpd_response_t (*minihttpd_handler_t)(const char *path, void *user_data);

struct minihttpd {
	int listen_fd;
	minihttpd_handler_t handler;
	void *user_data;
	int running;
};

typedef struct minihttpd minihttpd_t;

typedef struct {
	int client_fd;
	minihttpd_t *server;
} conn_ctx_t;

minihttpd_t *minihttpd_init(int port, minihttpd_handler_t handler);
void minihttpd_run(minihttpd_t *server);
void minihttpd_stop(minihttpd_t *server);
void minihttpd_free(minihttpd_t *server);

#endif
