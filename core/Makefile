SRC := $(wildcard ./src/*.c)
INCLUDES := -I./include

CC := gcc
CFLAGS := -Wall $(INCLUDES)
TARGET := main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)