CC = gcc
CFLAGS = -Wall -Wextra -I./src -I./include
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build

all: $(BIN_DIR)/server

$(BIN_DIR)/server: $(SRC_DIR)/http/http.c $(SRC_DIR)/main.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)
