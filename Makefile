VERSION_MAJOR = 0
VERSION_MINOR = 1
VERSION_PATCH = 0
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags xcb check` --coverage -g -O0 -DVERSION=\"$(VERSION)\" -I$(SRC_DIR)
LIBS = `pkg-config --libs xcb check`
LDFLAGS = -lX11 -lxcb-ewmh -lxcb -lxcb-keysyms -lxcb-xkb -lxcb-icccm --coverage

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRCS = $(SRC_DIR)/menu.c $(SRC_DIR)/input_handler.c $(SRC_DIR)/example_menu.c $(SRC_DIR)/main.c $(SRC_DIR)/x11_focus.c
TESTS = $(TEST_DIR)/test_menu.c $(TEST_DIR)/test_input_handler.c
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS = $(TESTS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/test_menu $(BUILD_DIR)/test_input_handler $(BUILD_DIR)/x11_input_example

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/x11_input_example: $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/test_menu: $(BUILD_DIR)/test_menu.o $(BUILD_DIR)/example_menu.o $(BUILD_DIR)/menu.o $(BUILD_DIR)/x11_focus.o | $(BUILD_DIR)
	$(CC) $^ -o $@ $(LIBS) $(LDFLAGS)

$(BUILD_DIR)/test_input_handler: $(BUILD_DIR)/test_input_handler.o $(BUILD_DIR)/input_handler.o $(BUILD_DIR)/example_menu.o $(BUILD_DIR)/menu.o $(BUILD_DIR)/x11_focus.o | $(BUILD_DIR)
	$(CC) $^ -o $@ $(LIBS) $(LDFLAGS)

run_tests: all
	$(BUILD_DIR)/test_menu
	$(BUILD_DIR)/test_input_handler

coverage: run_tests
	lcov --capture --directory . --output-file $(BUILD_DIR)/coverage.info
	genhtml $(BUILD_DIR)/coverage.info --output-directory $(BUILD_DIR)/coverage_report
	@echo "Coverage report generated in $(BUILD_DIR)/coverage_report/index.html"

clean:
	rm -rf $(BUILD_DIR)
	rm -rf coverage_report
	rm -rf coverage.info
	rm -f *.gcda *.gcno src/*.gcno src/*.gcda

run:
	sleep 1 && echo 3 && sleep 1 && echo 2 && sleep 1 && echo 1 && $(BUILD_DIR)/x11_input_example

.PHONY: all run_tests clean coverage
