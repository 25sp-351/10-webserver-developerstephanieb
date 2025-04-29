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

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  char buffer[BUFFER_SIZE];

  ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
  if (bytes_received <= 0) {
    if (verbose) {
      printf("Client disconnected or error during read.\n");
    }
    close(client_fd);
    return NULL;
  }

  buffer[bytes_received] = '\0';

  if (verbose) {
    printf("Received request:\n%s\n", buffer);
  }

  char method[8];
  char path[1024];
  char version[16];

  char *line = strtok(buffer, "\r\n");
  if (line) {
    if (sscanf(line, "%7s %1023s %15s", method, path, version) == 3) {
      if (verbose) {
        printf("Parsed Request:\n");
        printf("Method: %s\n", method);
        printf("Path: %s\n", path);
        printf("Version: %s\n", version);
      }
      if (strcmp(method, "GET") != 0) {
        if (verbose) {
          printf("Unsupported method: %s\n", method);
        }
        close(client_fd);
        if (verbose) {
          printf("Client disconnected\n");
        }
        return NULL;
      }
    } else {
      if (verbose) {
        printf("Malformed request line.\n");
      }
      close(client_fd);
      if (verbose) {
        printf("Client disconnected\n");
      }
      return NULL;
    }
  } else {
    if (verbose) {
      printf("No request line found.\n");
    }
    close(client_fd);
    if (verbose) {
      printf("Client disconnected\n");
    }
    return NULL;
  }

  char *header_line;
  while ((header_line = strtok(NULL, "\r\n")) != NULL &&
         strlen(header_line) > 0) {
    if (verbose) {
      printf("Header: %s\n", header_line);
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