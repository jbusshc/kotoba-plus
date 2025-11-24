APP_NAME = cli-tango
SRC = src/
BIN = bin/
MAIN = main.c

INCLUDE = -Iinclude/ -I../libtango/include 
LIBS = -L../libtango/build/lib/ -ltango -lcjson -lsqlite3

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 $(INCLUDE)
.PHONY: all clean

all: $(BIN)$(APP_NAME)

$(BIN)$(APP_NAME): $(SRC)$(MAIN)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -rf $(BIN)

run: 
	cd $(BIN) && ./cli-tango

build-lib: 
	cd ../libtango/build && cmake .. && ninja

build-db:
	cd ../xml-parser && make && make run