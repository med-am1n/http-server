#ifndef HTTP_H
#define HTTP_H

typedef struct {
  char *method;
  char *uri;
  char *host;
  char *user_agent;
} http_req_t;

int parse_http_request(char *msg, http_req_t *req);

void send_response(int sockfd, int status_code, const char *status_text,
                   const char *content_type, const char *body);

void handle_request(int sockfd);

#endif /* HTTP_H */
