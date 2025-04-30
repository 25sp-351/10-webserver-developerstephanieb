#ifndef STATIC_HANDLER_H
#define STATIC_HANDLER_H

void handle_static(int client_fd, const char *url_path);
void handle_sleep(int client_fd, const char *path);

#endif