#include "request.h"
#include <stdio.h>
#include <string.h>

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
  }

  return 0;
}
