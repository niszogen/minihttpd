#include "minihttpd.h"
#include "mh_platform.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define SERVER_STRING "Server: libminihttpd\r\n"

static int build_header(const minihttpd_response_t *res, char *out, size_t cap) {
	int has_body = res->file_fd >= 0 || res->content;
	const char *status = has_body ? "200 OK" : "404 Not Found";
	const char *mime_type = has_body ? res->mime_type : "text/plain";
	size_t len = res->file_fd >= 0 ? res->file_len : has_body ? res->content_len
															  : strlen("Not Found\n");

	int header_len = snprintf(out, cap,
							  "HTTP/1.1 %s\r\n"
							  "Content-Type: %s\r\n"
							  "Content-Length: %zu\r\n" SERVER_STRING
							  "\r\n",
							  status, mime_type, len);
	if (header_len < 0 || header_len >= (int)cap) {
		header_len = cap - 1;
	}

	return header_len;
}

void *handle_request(void *arg) {
	conn_ctx_t *conn = arg;
	int client_socket = conn->client_fd;
	minihttpd_t *server = conn->server;
	free(conn);

	char buffer[BUFFER_SIZE];
	ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

	if (bytes_received <= 0) {
		close(client_socket);
		return NULL;
	}
	buffer[bytes_received] = '\0';

	char url_encoded_name[BUFFER_SIZE];
	if (sscanf(buffer, "GET %s HTTP/1", url_encoded_name) != 1) {
		close(client_socket);
		return NULL;
	}

	printf("[INFO] Client connected from: %s\n", url_encoded_name);

	minihttpd_response_t res = server->handler(url_encoded_name, server->user_data);

	char header[256];
	int hdr_len = build_header(&res, header, sizeof(header));

	if (res.file_fd >= 0) {
		mh_sendfile(client_socket, header, (size_t)hdr_len, res.file_fd, res.file_len);
		close(res.file_fd);
	} else {
		const char *body = res.content ? res.content : "Not found\n";
		size_t body_len = res.content ? res.content_len : strlen(body);
		mh_write_all(client_socket, header, (size_t)hdr_len);
		mh_write_all(client_socket, body, body_len);
		if (res.owns_content)
			free((void *)res.content);
	}

	close(client_socket);
	return NULL;
}

minihttpd_t *minihttpd_init(int port, minihttpd_handler_t handler) {
	minihttpd_t *server = malloc(sizeof(minihttpd_t));
	if (!server)
		return NULL;
	struct sockaddr_in address;

	server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server->listen_fd < 0) {
		free(server);
		return NULL;
	}
	server->handler = handler;
	server->running = 0;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(server->listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		fprintf(stderr, "[ERR] Bind failed\n");
		close(server->listen_fd);
		free(server);
		return NULL;
	};

	if (listen(server->listen_fd, 10) < 0) {
		fprintf(stderr, "[ERR] Listen failed\n");
		close(server->listen_fd);
		free(server);
		return NULL;
	}

	return server;
}

void minihttpd_run(minihttpd_t *server) {
	server->running = 1;

	while (server->running) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_socket = -1;

		client_socket = accept(server->listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);

		if (client_socket < 0) {
			if (!server->running)
				break;
			fprintf(stderr, "[ERR] Accept failed\n");
			continue;
		}

		conn_ctx_t *conn = malloc(sizeof(conn_ctx_t));
		if (!conn) {
			close(client_socket);
			continue;
		}
		conn->client_fd = client_socket;
		conn->server = server;

		pthread_t thread_id;
		pthread_create(&thread_id, NULL, handle_request, conn);
		pthread_detach(thread_id);
	}
}

void minihttpd_stop(minihttpd_t *server) {
	if (!server)
		return;
	server->running = 0;
	close(server->listen_fd);
}

void minihttpd_free(minihttpd_t *server) {
	if (!server)
		return;
	close(server->listen_fd);
	free(server);
}
