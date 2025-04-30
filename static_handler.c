#include "static_handler.h"
#include "response.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

void handle_sleep(int client_fd, const char *path) {
  int seconds = 0;
  if (sscanf(path, "/sleep/%d", &seconds) != 1 || seconds < 0 || seconds > 10) {
    const char *error =
        "<html><body>Invalid sleep time (0–10 seconds allowed)</body></html>";
    send_response(client_fd, 400, "text/html", error, strlen(error));
    return;
  }

  sleep(seconds);

  char body[128];
  snprintf(body, sizeof(body),
           "<html><body>Slept for %d second(s)</body></html>", seconds);

  send_response(client_fd, 200, "text/html", body, strlen(body));
}
