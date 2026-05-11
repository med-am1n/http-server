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

void send_response(int sockfd, int status_code, const char *status_text,
                   const char *content_type, const char *body) {
  char response[2048];

  int body_length = strlen(body);

  int len = snprintf(response, sizeof(response),
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %d\r\n"
                     "\r\n"
                     "%s",
                     status_code, status_text, content_type, body_length, body);

  send(sockfd, response, len, 0);
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

    send_response(sockfd, 405, "Method Not Allowed", "text/plain",
                  "Method Not Allowed");

    return;
  }

  if (strcmp(req.uri, "/") == 0) {

    send_response(sockfd, 200, "OK", "text/html",
                  "<h1>Hello from my C server</h1>");

    return;
  }

  send_response(sockfd, 404, "Not Found", "text/plain", "404 Not Found");
}
