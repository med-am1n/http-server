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
      perror("Error accepting connection");
      continue;
    }

    addr_to_str((struct sockaddr *)&client_addr, ipstr);

    printf("server: got connection from %s\n", ipstr);

    if (fork() == 0) {

      close(server_sockfd); // Child does NOT need the listening socket.

      if (send(client_sockfd, "Hello, world!", 13, 0) == -1) {
        perror("Error sending message");
      }

      close(client_sockfd);

      exit(0);
    }

    close(client_sockfd);
  }

  return 0;
}