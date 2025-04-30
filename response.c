#include "response.h"
#include "request.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
  case 403:
    status_text = "Forbidden";
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
