CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = webserver
SRC = webserver.c request.c response.c calc_handler.c static_handler.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)