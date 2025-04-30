#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

int send_response(int client_fd, int status_code, const char *content_type,
                  const char *body, size_t body_length);
const char *get_mime_type(const char *filename);

#endif