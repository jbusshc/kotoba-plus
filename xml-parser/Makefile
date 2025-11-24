CC := gcc
CFLAGS := -Iinclude -Wall -Wextra -O2 $(shell xml2-config --cflags) 
LDFLAGS := $(shell xml2-config --libs) -lsqlite3
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin
BIN := $(BIN_DIR)/gotango-parser

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Create directories if they don't exist
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: all clean run

run:
	cd $(BIN_DIR) && ./gotango-parser && mv tango.db ../cli-tango/bin/tango.db

all: $(BIN)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)