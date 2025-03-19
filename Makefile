VERSION_MAJOR = 0
VERSION_MINOR = 1
VERSION_PATCH = 0
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags xcb check cairo cairo-xcb` --coverage -g -O0 -DVERSION=\"$(VERSION)\" -I$(SRC_DIR)
LIBS = `pkg-config --libs xcb check cairo cairo-xcb`
LDFLAGS = -lX11 -lxcb-ewmh -lxcb -lxcb-keysyms -lcairo -lxcb-xkb -lxcb-icccm --coverage
# LDFLAGS = -lX11 -lxcb-ewmh -lxcb -lxcb-keysyms -lxcb-xkb -lxcb-icccm --coverage
# CFLAGS = -Wall -Wextra -I/usr/include/cairo -I/usr/include/xcb
# LDFLAGS = -lxcb -lxcb-ewmh -lxcb-icccm -lcairo -lcairo-xcb

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(SRC_DIR)/menu.c $(SRC_DIR)/input_handler.c $(SRC_DIR)/example_menu.c $(SRC_DIR)/main.c $(SRC_DIR)/x11_focus.c $(SRC_DIR)/cairo_menu.c $(SRC_DIR)/x11_window.c $(SRC_DIR)/window_menu.c
TESTS = $(TEST_DIR)/test_menu.c $(TEST_DIR)/test_input_handler.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS = $(TESTS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%.o)
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = $(BUILD_DIR)/x11-window-menu

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	$(TARGET)
