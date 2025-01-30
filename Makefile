ARCH ?= x86_64 

SRC_DIR := src
HEADERS_DIR := headers
BIN_DIR := bin

CFLAGS := -g

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SOURCES))

TARGET := $(BIN_DIR)/answering_machine

ifeq ($(ARCH), x86_64)
	CC := gcc
	INCLUDES := -I$(HEADERS_DIR) $(shell pkg-config --cflags libpjproject)
	LIBS := $(shell pkg-config --libs --static libpjproject) 
else ifeq ($(ARCH), arm) 
	CC := arm-marvell-linux-gnueabi-gcc
	INCLUDES := -I$(HEADERS_DIR) $(shell pkg-config --cflags ~/pjproject-2.15.1/arm_build/lib/pkgconfig/libpjproject.pc)
	LIBS := $(shell pkg-config --libs --static ~/pjproject-2.15.1/arm_build/lib/pkgconfig/libpjproject.pc) -lpthread -lrt
endif

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
