#include "handler.h"

#include "main.h"

char *get_mime_type(const char *path) {
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
	else if (strcasecmp(dot, ".webp") == 0)
		return "image/webp";
	else if (strcasecmp(dot, ".jxl") == 0)
		return "image/jxl";
	else if (strcasecmp(dot, ".txt") == 0)
		return "text/plain";
	else
		return "application/octet-stream";
}

minihttpd_response_t handler(const char *path, void *user_data) {
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
