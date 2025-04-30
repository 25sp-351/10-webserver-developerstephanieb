#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

#define BUFFER_SIZE 1024
#define PIPELINE_BUFFER_SIZE 8192

typedef struct {
  char method[8];
  char path[1024];
  char version[16];
} HttpRequest;

int parse_http_request(const char *raw, HttpRequest *req);

#endif