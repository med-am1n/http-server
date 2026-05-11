#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490"
#define BACKLOG 10

/*
 * Reap terminated child processes to prevent zombies.
 * Called automatically when a child exits (SIGCHLD).
 */
void sigchld_handler(int s) {
  (void)s;

  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

void setup_sigchld_handler(void) {
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;

  sigemptyset(&sa.sa_mask);

  sa.sa_flags = SA_RESTART; // 0

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("Error setting up SIGCHLD handler");
    exit(1);
  }
}

/*
 * Convert IPv4 or IPv6 socket address into a printable IP string.
 */
void addr_to_str(struct sockaddr *addr, char *str) {
  void *src_addr;
  if (addr->sa_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
    src_addr = &ipv4->sin_addr;
  } else {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;
    src_addr = &ipv6->sin6_addr;
  }
  inet_ntop(addr->sa_family, src_addr, str, INET6_ADDRSTRLEN);
}

/*
 * Create, bind, and listen on a TCP server socket.
 *
 * Returns:
 *   server socket file descriptor on success
 *   -1 on failure
 */
int create_server_socket(void) {
  struct addrinfo hints;
  struct addrinfo *servinfo;

  int sockfd;
  int yes = 1;
  int rv;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  sockfd =
      socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

  if (sockfd == -1) {
    perror("Error creating socket");
    freeaddrinfo(servinfo);
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    perror("Error setting socket options");
    close(sockfd);
    freeaddrinfo(servinfo);
    return -1;
  }

  if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    perror("Error binding socket");
    close(sockfd);
    freeaddrinfo(servinfo);
    return -1;
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("Error listening on socket");
    close(sockfd);
    freeaddrinfo(servinfo);
    return -1;
  }

  freeaddrinfo(servinfo); // done with it

  return sockfd;
}

typedef struct {
  char *method;
  char *uri;
  char *host;
  char *user_agent;
} http_req_t;

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

int main(void) {
  int server_sockfd;

  char ipstr[INET6_ADDRSTRLEN];

  if ((server_sockfd = create_server_socket()) == -1) {
    exit(1);
  }

  setup_sigchld_handler();

  printf("server: waiting for connections...\n");

  // accept incoming connections
  struct sockaddr_storage client_addr = {0};
  socklen_t addr_size;

  int client_sockfd;

  while (1) {

    addr_size = sizeof(client_addr);

    client_sockfd =
        accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_size);

    if (client_sockfd == -1) {
      if (errno == EINTR) {
        continue; // If accept() was interrupted by a signal (child exits,
                  // kernel sends SIGCHLD, retry
      }
      perror("Error accepting connection");
      continue;
    }

    addr_to_str((struct sockaddr *)&client_addr, ipstr);
    printf("server: got connection from %s\n", ipstr);

    if (fork() == 0) {

      close(server_sockfd); // Child does NOT need the listening socket.

      handle_request(client_sockfd);

      close(client_sockfd);

      exit(0);
    }

    close(client_sockfd);
  }

  return 0;
}