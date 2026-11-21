
CC       := gcc
TARGET   := myproject
SRC      := src/main.c

WARNINGS := -Wall -Wextra -Wpedantic -Werror \
            -Wconversion -Wsign-conversion -Wshadow \
            -Wformat=2 -Wundef -Wstrict-prototypes \
            -Wmissing-prototypes -Wold-style-definition

CPPFLAGS :=
LDLIBS   := -pthread

# libcurl:
# CPPFLAGS += $(shell pkg-config --cflags libcurl)
# LDLIBS   += $(shell pkg-config --libs libcurl)

# SQLite3:
# CPPFLAGS += $(shell pkg-config --cflags sqlite3)
# LDLIBS   += $(shell pkg-config --libs sqlite3)

.PHONY: all debug sanitize release clean

all: debug

debug:
	$(MAKE) MODE=debug build

sanitize:
	$(MAKE) MODE=sanitize build

release:
	$(MAKE) MODE=release build

ifeq ($(MODE),debug)
    CFLAGS := -std=c17 $(WARNINGS) -g -O0
else ifeq ($(MODE),sanitize)
    CFLAGS := -std=c17 $(WARNINGS) -g -O1 \
              -fsanitize=address,undefined \
              -fno-omit-frame-pointer
    LDLIBS += -fsanitize=address,undefined
else ifeq ($(MODE),release)
    CFLAGS := -std=c17 $(WARNINGS) -O2 -DNDEBUG
else
    CFLAGS := -std=c17 $(WARNINGS) -g -O0
endif

BUILD_DIR := build/$(if $(MODE),$(MODE),debug)
OBJ := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: build
build: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -pthread -c $< -o $@

clean:
	rm -rf build

# Использование
# make debug [/usr/bin/clang-15, /usr/bin/gcc-11, /usr/bin/gcc-12]
# запуск: ./build/debug/myproject

# make sanitize [/usr/bin/clang-15, /usr/bin/gcc-11, /usr/bin/gcc-12]
# запуск: ./build/sanitize/myproject

# make release [/usr/bin/clang-15, /usr/bin/gcc-11, /usr/bin/gcc-12]
# запуск: ./build/release/myproject

# make clean