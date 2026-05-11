#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int parse_http_request(char *msg, http_req_t *req) {
  char *line;
  char *method;
  char *uri;
  char *version;

  // First line
  line = strtok(msg, "\r\n");

  if (line == NULL)
    return -1;

  // Parse request line
  method = strtok(line, " ");
  uri = strtok(NULL, " ");
  version = strtok(NULL, " ");

  if (!method || !uri || !version)
    return -1;

  req->method = method;
  req->uri = uri;

  // Parse headers
  while ((line = strtok(NULL, "\r\n")) != NULL) {

    // Empty line => end of headers
    if (strlen(line) == 0)
      break;

    char *colon = strchr(line, ':');

    if (!colon)
      continue;

    *colon = '\0';

    char *key = line;
    char *value = colon + 1;

    // Skip leading spaces
    while (*value == ' ')
      value++;

    if (strcmp(key, "Host") == 0) {
      req->host = value;
    }

    if (strcmp(key, "User-Agent") == 0) {
      req->user_agent = value;
    }
  }

  return 0;
}
char *serialize_http_response(http_res_t *res) {

  int buffer_size = res->content_length + 1024;

  char *buffer = calloc(buffer_size, sizeof(char));

  if (!buffer)
    return NULL;

  snprintf(buffer, buffer_size,
           "%s %d %s\r\n"
           "Content-Type: %s\r\n"
           "Content-Length: %d\r\n"
           "Connection: close\r\n"
           "\r\n"
           "%s",

           res->version, res->status_code, res->reason,

           res->content_type, res->content_length,

           res->body);

  return buffer;
}
void send_http_response(int sockfd, http_res_t *res) {

  char *raw = serialize_http_response(res);

  if (!raw)
    return;

  send(sockfd, raw, strlen(raw), 0);

  free(raw);
}

void handle_request(int sockfd) {
  char buffer[1024];

  int bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_read == -1) {
    perror("Error receiving message");
    close(sockfd);
    exit(1);
  }

  http_req_t req = {0};
  buffer[bytes_read] = '\0';

  if (parse_http_request(buffer, &req) == 1) {
    exit(1);
  }
  if (strcmp(req.method, "GET") != 0) {

    http_res_t res = {.version = "HTTP/1.1",
                      .status_code = 405,
                      .reason = "Method Not Allowed",
                      .content_type = "text/plain",
                      .body = "Method Not Allowed"};

    res.content_length = strlen(res.body);

    send_http_response(sockfd, &res);

    return;
  }

  if (strcmp(req.uri, "/") == 0) {

    http_res_t res = {.version = "HTTP/1.1",
                      .status_code = 200,
                      .reason = "OK",
                      .content_type = "text/html",
                      .body = "<h1>Hello from C server</h1>"};

    res.content_length = strlen(res.body);

    send_http_response(sockfd, &res);

    return;
  }

  http_res_t res = {.version = "HTTP/1.1",
                    .status_code = 404,
                    .reason = "Not Found",
                    .content_type = "text/plain",
                    .body = "404 Not Found"};

  res.content_length = strlen(res.body);

  send_http_response(sockfd, &res);
}
