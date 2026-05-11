CC = gcc
CFLAGS = -Wall -Wextra -I./src
SRC_DIR = src

all: main client

main: $(SRC_DIR)/main.c $(SRC_DIR)/http.c
	$(CC) $(CFLAGS) $^ -o $@

client: $(SRC_DIR)/client.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f main client
