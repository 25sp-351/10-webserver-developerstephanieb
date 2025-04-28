#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 1024
#define LISTEN_BACKLOG 10
int verbose = 0;

void serve_static(int client_fd, const char *path);
void serve_calc(int client_fd, const char *path);

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  char buffer[BUFFER_SIZE];

  ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);

  if (bytes_received > 0) {
    buffer[bytes_received] = '\0';

    if (verbose) {
      printf("Received Request:\n%s\n", buffer);
    }

    char method[8];
    char path[1024];
    char version[16];

    sscanf(buffer, "%s %s %s", method, path, version);

    if (verbose) {
      printf("Method: %s\n", method);
      printf("Path: %s\n", path);
      printf("Version: %s\n", version);
    }

    if (strcmp(method, "GET") != 0) {
      const char *error_response = "HTTP/1.1 405 Method Not Allowed\r\n"
                                   "Content-Length: 0\r\n"
                                   "\r\n";
      write(client_fd, error_response, strlen(error_response));
      close(client_fd);
      return NULL;
    }

    if (strncmp(path, "/static", 7) == 0) {
      serve_static(client_fd, path);
    } else if (strncmp(path, "/calc", 5) == 0) {
      serve_calc(client_fd, path);
    } else {
      const char *not_found_response = "HTTP/1.1 404 Not Found\r\n"
                                       "Content-Length: 0\r\n"
                                       "\r\n";
      write(client_fd, not_found_response, strlen(not_found_response));
    }
  }

  close(client_fd);
  if (verbose) {
    printf("Client disconnected\n");
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      port = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = 1;
    } else {
      printf("Usage: %s [-p port] [-v]\n", argv[0]);
    }
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in socket_address;
  memset(&socket_address, '\0', sizeof(socket_address));
  socket_address.sin_family = AF_INET;
  socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
  socket_address.sin_port = htons(port);

  if (verbose) {
    printf("Binding to port %d...\n", port);
  }

  if (bind(socket_fd, (struct sockaddr *)&socket_address,
           sizeof(socket_address)) < 0) {
    perror("bind");
    close(socket_fd);
    return 1;
  }

  if (listen(socket_fd, LISTEN_BACKLOG) < 0) {
    perror("listen");
    close(socket_fd);
    return 1;
  }

  if (verbose) {
    printf("Server listening on port %d...\n", port);
  }

  while (1) {
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    int *client_fd_buf = malloc(sizeof(int));
    if (!client_fd_buf) {
      perror("malloc");
      continue;
    }

    *client_fd_buf = accept(socket_fd, (struct sockaddr *)&client_address,
                            &client_address_len);

    if (*client_fd_buf < 0) {
      perror("accept");
      free(client_fd_buf);
      continue;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, sizeof(client_ip));
    if (verbose) {
      printf("Accepted connection from %s:%d\n", client_ip,
             ntohs(client_address.sin_port));
    }
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, client_fd_buf);
    pthread_detach(thread);
  }

  close(socket_fd);
  return 0;
}