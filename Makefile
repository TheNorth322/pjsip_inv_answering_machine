CC := gcc

SRC_DIR := src
HEADERS_DIR := headers
BIN_DIR := bin

INCLUDES := -I$(HEADERS_DIR) $(shell pkg-config --cflags libpjproject)
LIBS := $(shell pkg-config --libs --static libpjproject) 
CFLAGS := -g

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SOURCES))

TARGET := $(BIN_DIR)/answering_machine

all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@mkdir -p $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LIBS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	@rm -rf $(BIN_DIR)

.PHONY: all clean
