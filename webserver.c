#include "calc_handler.h"
#include "request.h"
#include "response.h"
#include "static_handler.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PORT 80
#define LISTEN_BACKLOG 10
int verbose = 0;

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  char buffer[PIPELINE_BUFFER_SIZE];
  size_t buf_len = 0;

  while (1) {
    ssize_t bytes_read =
        read(client_fd, buffer + buf_len, sizeof(buffer) - buf_len - 1);
    if (bytes_read <= 0) {
      if (verbose)
        printf("Client disconnected or error.\n");
      break;
    }

    buf_len += bytes_read;
    buffer[buf_len] = '\0';

    char *start = buffer;
    while (1) {
      char *end_of_request = strstr(start, "\r\n\r\n");
      if (!end_of_request)
        break;

      size_t request_len = end_of_request - start + 4;
      char request_buf[BUFFER_SIZE];
      strncpy(request_buf, start, request_len);
      request_buf[request_len] = '\0';

      HttpRequest req;
      if (parse_http_request(request_buf, &req) == 0) {
        if (verbose)
          printf("Parsed pipelined request: %s\n", req.path);

        if (strncmp(req.path, "/calc/", 6) == 0) {
          handle_calc(client_fd, req.path);
        } else if (strncmp(req.path, "/static/", 8) == 0) {
          handle_static(client_fd, req.path);
        } else if (strncmp(req.path, "/sleep/", 7) == 0) {
          handle_sleep(client_fd, req.path);
        } else {
          const char *not_found =
              "<html><body><h1>404 Not Found</h1></body></html>";
          send_response(client_fd, 404, "text/html", not_found,
                        strlen(not_found));
        }
      }

      start = end_of_request + 4;
    }

    buf_len = strlen(start);
    memmove(buffer, start, buf_len);
  }

  close(client_fd);
  if (verbose)
    printf("Client disconnected\n");
  return NULL;
}

int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;

  for (int ii = 1; ii < argc; ii++) {
    if (strcmp(argv[ii], "-p") == 0 && ii + 1 < argc) {
      port = atoi(argv[++ii]);
    } else if (strcmp(argv[ii], "-v") == 0) {
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

  if (verbose)
    printf("Server listening on port %d...\n", port);

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
