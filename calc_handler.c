#include "calc_handler.h"
#include "response.h"
#include <stdio.h>
#include <string.h>

void handle_calc(int client_fd, const char *path) {
  char op[4];
  int num1;
  int num2;

  if (sscanf(path, "/calc/%3[^/]/%d/%d", op, &num1, &num2) != 3) {
    const char *error = "<html><body>Malformed /calc path</body></html>";
    send_response(client_fd, 400, "text/html", error, strlen(error));
    return;
  }

  int result = 0;
  int valid = 1;
  char symbol[4];

  if (strcmp(op, "add") == 0) {
    result = num1 + num2;
    strcpy(symbol, "+");
  } else if (strcmp(op, "mul") == 0) {
    result = num1 * num2;
    strcpy(symbol, "*");
  } else if (strcmp(op, "div") == 0) {
    if (num2 == 0) {
      const char *error = "<html><body>Division by zero</body></html>";
      send_response(client_fd, 400, "text/html", error, strlen(error));
      return;
    }
    result = num1 / num2;
    strcpy(symbol, "/");
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
           symbol, num2, result);

  send_response(client_fd, 200, "text/html", body, strlen(body));
}
