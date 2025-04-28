CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = webserver
SRC = webserver.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)