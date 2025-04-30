#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PORT 80
#define BUFFER_SIZE 1024
#define LISTEN_BACKLOG 10
int verbose = 0;

typedef struct {
  char method[8];
  char path[1024];
  char version[16];
} HttpRequest;

void *handle_client(void *arg);
int parse_http_request(const char *raw, HttpRequest *req);
int send_response(int client_fd, int status_code, const char *content_type,
                  const char *body, size_t body_length);
void handle_calc(int client_fd, const char *path);
void handle_static(int client_fd, const char *url_path);
const char *get_mime_type(const char *filename);

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

  HttpRequest req;
  if (parse_http_request(buffer, &req) != 0) {
    if (verbose) {
      printf("Invalid or unsupported HTTP request.\n");
    }
    close(client_fd);
    if (verbose) {
      printf("Client disconnected\n");
    }
    return NULL;
  }

  if (verbose) {
    printf("Parsed Request:\n");
    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);
  }

  if (strncmp(req.path, "/calc/", 6) == 0) {
    handle_calc(client_fd, req.path);
    close(client_fd);
    if (verbose) {
      printf("Client disconnected\n");
    }
    return NULL;
  }

  if (strncmp(req.path, "/static/", 8) == 0) {
    handle_static(client_fd, req.path);
    close(client_fd);
    if (verbose)
      printf("Client disconnected\n");
    return NULL;
  }

  const char *not_found = "<html><body><h1>404 Not Found</h1></body></html>";
  send_response(client_fd, 404, "text/html", not_found, strlen(not_found));

  close(client_fd);
  if (verbose) {
    printf("Client disconnected\n");
  }
  return NULL;
}

int parse_http_request(const char *raw, HttpRequest *req) {
  char temp[BUFFER_SIZE];
  strncpy(temp, raw, BUFFER_SIZE - 1);
  temp[BUFFER_SIZE - 1] = '\0';

  char *line = strtok(temp, "\r\n");
  if (!line)
    return -1;

  if (sscanf(line, "%7s %1023s %15s", req->method, req->path, req->version) !=
      3) {
    return -1;
  }

  if (strcmp(req->version, "HTTP/1.1") != 0) {
    return -1;
  }

  if (strcmp(req->method, "GET") != 0) {
    return -1;
  }

  char *header_line;
  while ((header_line = strtok(NULL, "\r\n")) != NULL &&
         strlen(header_line) > 0) {
    if (verbose) {
      printf("Header: %s\n", header_line);
    }
  }

  return 0;
}

int send_response(int client_fd, int status_code, const char *content_type,
                  const char *body, size_t body_length) {
  char header[BUFFER_SIZE];

  const char *status_text;
  switch (status_code) {
  case 200:
    status_text = "OK";
    break;
  case 400:
    status_text = "Bad Request";
    break;
  case 404:
    status_text = "Not Found";
    break;
  case 405:
    status_text = "Method Not Allowed";
    break;
  case 500:
    status_text = "Internal Server Error";
    break;
  default:
    status_text = "Unknown";
    break;
  }

  int header_length =
      snprintf(header, sizeof(header),
               "HTTP/1.1 %d %s\r\n"
               "Content-Type: %s\r\n"
               "Content-Length: %zu\r\n"
               "Connection: close\r\n"
               "\r\n",
               status_code, status_text, content_type, body_length);

  if (write(client_fd, header, header_length) < 0) {
    perror("write header");
    return -1;
  }

  if (write(client_fd, body, body_length) < 0) {
    perror("write body");
    return -1;
  }

  return 0;
}

void handle_calc(int client_fd, const char *path) {
  char op[4];
  int num1, num2;

  if (sscanf(path, "/calc/%3[^/]/%d/%d", op, &num1, &num2) != 3) {
    const char *error = "<html><body>Malformed /calc path</body></html>";
    send_response(client_fd, 400, "text/html", error, strlen(error));
    return;
  }

  int result = 0;
  int valid = 1;
  char operation[16];

  if (strcmp(op, "add") == 0) {
    result = num1 + num2;
    strcpy(operation, "+");
  } else if (strcmp(op, "mul") == 0) {
    result = num1 * num2;
    strcpy(operation, "*");
  } else if (strcmp(op, "div") == 0) {
    if (num2 == 0) {
      const char *error = "<html><body>Division by zero</body></html>";
      send_response(client_fd, 400, "text/html", error, strlen(error));
      return;
    }
    result = num1 / num2;
    strcpy(operation, "/");
  } else {
    valid = 0;
  }

  if (!valid) {
    const char *error = "<html><body>Unsupported operation</body></html>";
    send_response(client_fd, 400, "text/html", error, strlen(error));
    return;
  }

  char body[256];
  snprintf(body, sizeof(body), "<html><body>%d %s %d = %d</body></html>", num1,
           operation, num2, result);

  send_response(client_fd, 200, "text/html", body, strlen(body));
}

void handle_static(int client_fd, const char *url_path) {
  const char *relative_path = url_path + 7;
  if (strstr(relative_path, "..")) {
    const char *error = "<html><body>403 Forbidden</body></html>";
    send_response(client_fd, 403, "text/html", error, strlen(error));
    return;
  }

  char file_path[1024];
  snprintf(file_path, sizeof(file_path), "static%s", relative_path);

  int file_fd = open(file_path, O_RDONLY);
  if (file_fd < 0) {
    const char *error = "<html><body>404 File Not Found</body></html>";
    send_response(client_fd, 404, "text/html", error, strlen(error));
    return;
  }

  struct stat file_stat;
  if (fstat(file_fd, &file_stat) < 0) {
    close(file_fd);
    const char *error = "<html><body>500 Internal Server Error</body></html>";
    send_response(client_fd, 500, "text/html", error, strlen(error));
    return;
  }

  char *file_data = malloc(file_stat.st_size);
  if (!file_data) {
    close(file_fd);
    const char *error = "<html><body>500 Memory Error</body></html>";
    send_response(client_fd, 500, "text/html", error, strlen(error));
    return;
  }

  read(file_fd, file_data, file_stat.st_size);
  close(file_fd);

  const char *mime = get_mime_type(file_path);
  send_response(client_fd, 200, mime, file_data, file_stat.st_size);

  free(file_data);
}

const char *get_mime_type(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext)
    return "application/octet-stream";

  if (strcmp(ext, ".html") == 0)
    return "text/html";
  if (strcmp(ext, ".txt") == 0)
    return "text/plain";
  if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(ext, ".png") == 0)
    return "image/png";

  return "application/octet-stream";
}