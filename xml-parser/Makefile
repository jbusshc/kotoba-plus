CC := gcc

# =========================
# Rutas
# =========================
KOTOBA_ROOT := ../kotoba-core
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

BIN := $(BIN_DIR)/xml-parser.exe

# =========================
# Includes / Compile flags
# =========================
CFLAGS := \
	-Iinclude \
	-I$(KOTOBA_ROOT)/include \
	-Wall -Wextra -O2 \
	$(shell xml2-config --cflags)

# =========================
# Link flags
# =========================
# Kotoba DLL (import library)
KOTOBA_LDFLAGS := \
	-L$(KOTOBA_ROOT)/build/lib \
	-lkotobaplus

# Otras libs
LDFLAGS := \
	$(shell xml2-config --libs) \
	-Wl,--stack,16777216

# =========================
# Sources
# =========================
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# =========================
# Directorios
# =========================
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# =========================
# Build rules
# =========================
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ \
	    $(KOTOBA_LDFLAGS) \
	    $(LDFLAGS)

# =========================



# =========================
# Targets
# =========================
.PHONY: all clean run get-jmdict

all: $(BIN)

run:
	# Copiar la DLL en runtime
	cp $(KOTOBA_ROOT)/build/bin/kotobaplus.dll $(BIN_DIR)/
	cd $(BIN_DIR) && ./xml-parser.exe

clean:
	rm -rf $(OBJ_DIR) $(BIN)

get-jmdict:
	curl -O ftp://ftp.edrdg.org/pub/Nihongo/JMdict.gz
	gunzip JMdict.gz
