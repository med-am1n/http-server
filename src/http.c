#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void add_header(
    http_res_t *res,
    char *key,
    char *value)
{
  int i = res->header_count;

  res->headers[i].key = key;
  res->headers[i].value = value;

  res->header_count++;
}


int parse_http_request(char *msg, http_req_t *req)
{
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
  while ((line = strtok(NULL, "\r\n")) != NULL)
  {

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

    if (strcmp(key, "Host") == 0)
    {
      req->host = value;
    }

    if (strcmp(key, "User-Agent") == 0)
    {
      req->user_agent = value;
    }
  }

  return 0;
}

char *serialize_http_response(http_res_t *res)
{

  int buffer_size = strlen(res->body) + 4096;

  char *buffer = calloc(buffer_size, sizeof(char));

  if (!buffer)
    return NULL;

  int offset = 0;

  // Status line
  offset += snprintf(
      buffer + offset,
      buffer_size - offset,
      "%s %d %s\r\n",
      res->version,
      res->status_code,
      res->reason);

  // Headers
  for (int i = 0; i < res->header_count; i++)
  {

    offset += snprintf(
        buffer + offset,
        buffer_size - offset,
        "%s: %s\r\n",
        res->headers[i].key,
        res->headers[i].value);
  }

  // Empty line between headers/body
  offset += snprintf(
      buffer + offset,
      buffer_size - offset,
      "\r\n");

  // Body
  offset += snprintf(
      buffer + offset,
      buffer_size - offset,
      "%s",
      res->body);

  return buffer;
}

void send_http_response(int sockfd, http_res_t *res)
{

  char *raw = serialize_http_response(res);

  if (!raw)
    return;

  send(sockfd, raw, strlen(raw), 0);

  free(raw);
}

char *load_file(const char *path)
{

  FILE *file = fopen(path, "r");

  if (!file)
    return NULL;

  fseek(file, 0, SEEK_END);

  long size = ftell(file);

  rewind(file);

  char *buffer = calloc(size + 1, sizeof(char));

  if (!buffer)
  {
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, size, file);

  buffer[size] = '\0';

  fclose(file);

  return buffer;
}

http_res_t build_response(
    const char *status_code,
    const char *reason,
    const char *body,
    const char *content_type
)
{
    http_res_t res = {0};

    res.version = "HTTP/1.1";
    res.status_code = atoi(status_code);
    res.reason = (char *)reason;
    res.body = (char *)body;
    
    // char len_buf[32]; --- IGNORE --- header points to invalid memory after the function returns, so we need to allocate it on the heap
    char *len_str = calloc(32, sizeof(char));
    
    if (!len_str) return res;

    sprintf(len_str, "%zu", strlen(body));

    add_header(&res, "Content-Length", len_str);
    add_header(&res, "Content-Type", (char *)content_type);
    add_header(&res, "Connection", "close");

    return res;
}
void handle_request(int sockfd)
{
  char buffer[1024];

  int bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
  if (bytes_read == -1)
  {
    perror("Error receiving message");
    close(sockfd);
    exit(1);
  }

  http_req_t req = {0};
  buffer[bytes_read] = '\0';

  if (parse_http_request(buffer, &req) == 1)
  {
    exit(1);
  }
  if (strcmp(req.method, "GET") != 0)
  {

    http_res_t res = build_response("500", "Internal Server Error", "Internal Server Error", "text/html");
    send_http_response(sockfd, &res);

    return;
  }

  // Route handler
  if (strcmp(req.uri, "/") == 0)
  {

    char *html = load_file("./public/index.html");

    if (!html)
    {
      http_res_t res = build_response("500", "Internal Server Error", "Internal Server Error", "text/html");
      send_http_response(sockfd, &res);
      return;
    }

    http_res_t res = build_response("200", "OK", html, "text/html");

    send_http_response(sockfd, &res);

    return;
  }

  http_res_t res = build_response("404", "Not Found", "404 Not Found", "text/html");

  send_http_response(sockfd, &res);
}