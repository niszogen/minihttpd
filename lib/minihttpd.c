#include "minihttpd.h"

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

char *build_http_response(const minihttpd_response_t *res, size_t *response_len) {
	const char *status = res->content ? "200 OK" : "404 Not Found";
	const char *mime_type = res->content ? res->mime_type : "text/plain";
	const char *content = res->content ? res->content : "Not Found\n";
	size_t content_len = res->content ? res->content_len : strlen(content);

	char header[256];

	int header_len = snprintf(header, sizeof(header),
							  "HTTP/1.1 %s\r\n"
							  "Content-Type: %s\r\n"
							  "Content-Length: %zu\r\n" SERVER_STRING
							  "\r\n",
							  status, mime_type, content_len);
	if (header_len < 0 || header_len >= (int)sizeof(header)) {
		header_len = sizeof(header) - 1;
	}

	*response_len = (size_t)header_len + content_len;
	char *response = malloc(*response_len);
	if (!response)
		return NULL;
	memcpy(response, header, header_len);
	memcpy(response + header_len, content, content_len);

	return response;
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
	size_t response_len;
	char *response = build_http_response(&res, &response_len);

	if (response) {
		write(client_socket, response, response_len);
		free(response);
	}

	if (res.owns_content)
		free((void *)res.content);

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
