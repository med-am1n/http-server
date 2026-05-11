#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Create and connect a TCP client socket.
 *
 * Returns:
 *   connected socket fd on success
 *   -1 on failure
 */
int create_client_socket(const char *ip, const char *port) {
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct addrinfo *p;

  int sockfd;
  int rv;

  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET; // same as server (IPv4 only)
  hints.ai_socktype = SOCK_STREAM;

  rv = getaddrinfo(ip, port, &hints, &servinfo);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return -1;
  }

  // loop through results and connect to first valid one
  for (p = servinfo; p != NULL; p = p->ai_next) {

    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      perror("socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("connect");
      close(sockfd);
      continue;
    }

    break; // success
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    freeaddrinfo(servinfo);
    return -1;
  }

  freeaddrinfo(servinfo);
  return sockfd;
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
    exit(1);
  }

  const char *ip = argv[1];
  const char *port = argv[2];

  int sockfd;

  if ((sockfd = create_client_socket(ip, port)) == -1) {
    exit(1);
  }

  printf("client: connected to %s:%s\n", ip, port);

  char buf[4090];

  // send a request
  char req[256];

  snprintf(req, sizeof(req),
           "GET / HTTP/1.1\r\n"
           "Host: %s\r\n"
           "\r\n",
           ip);

  if (send(sockfd, req, strlen(req), 0) == -1) {
    perror("send");
    close(sockfd);
    exit(1);
  }

  printf("client: sent request\n");

  int bytes = recv(sockfd, buf, sizeof(buf) - 1, 0);
  if (bytes == -1) {
    perror("recv");
    close(sockfd);
    exit(1);
  }

  buf[bytes] = '\0';

  printf("server response:\n%s\n", buf);

  close(sockfd);

  return 0;
}