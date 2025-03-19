# Makefile for menu extension system

# Configuration
VERSION := $(shell grep 'VERSION' src/version.h | cut -d '"' -f2)
PREFIX ?= /usr/local

# Compiler and flags
CC := gcc
CFLAGS := -Wall # -Wextra -Werror  -std=c11 # -pedantic
CPPFLAGS := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
DEBUGFLAGS := -g3 -O0 -DDEBUG
RELEASEFLAGS := -O2 -DNDEBUG
SANITIZE := -fsanitize=address -fsanitize=undefined
COVERAGE := --coverage
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

# Include paths and libraries
INCLUDES := -I/usr/include/cairo -I/usr/include/xcb
LIBS := -lxcb -lxcb-ewmh -lcairo -lX11 -lm -lxcb-icccm -lgcov -lxcb-util

# Source directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
TEST_BUILD_DIR = $(BUILD_DIR)/tests

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(BUILD_DIR)/menu_animation.o $(filter-out $(BUILD_DIR)/menu_animation.o, $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o))
DEPS = $(OBJS:.o=.d)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(TEST_BUILD_DIR)/%)
TEST_DEPS = $(TEST_OBJS:.o=.d)

# Main binary
MAIN = $(BUILD_DIR)/menu_system

# Default target
all: $(MAIN)

# Create build directories
$(BUILD_DIR) $(TEST_BUILD_DIR):
	@mkdir -p $@

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# Compile test files
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# Link main binary
$(MAIN): $(OBJS)
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

# Build and run individual tests
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)
	@echo "TEST $@"
	@./$@

# Build and run all tests
.PHONY: tests test-sanitize test-coverage test-valgrind test-memcheck debug release install uninstall

tests: $(TEST_BINS)

# Debug build
debug: CFLAGS += $(DEBUGFLAGS)
debug: all

# Release build
release: CFLAGS += $(RELEASEFLAGS)
release: all

# Run tests with sanitizers
test-sanitize: CFLAGS += $(SANITIZE)
test-sanitize: LDFLAGS += $(SANITIZE)
test-sanitize: tests

# Run tests with valgrind
test-valgrind: debug
	@for test in $(TEST_BINS); do \
		echo "VALGRIND $$test"; \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $$test; \
	done

# Run tests with memory checker
test-memcheck: debug
	@for test in $(TEST_BINS); do \
		echo "MEMCHECK $$test"; \
		valgrind --tool=memcheck --leak-check=full --show-reachable=yes $$test; \
	done

# Generate test coverage
test-coverage: CFLAGS += $(COVERAGE)
test-coverage: LDFLAGS += $(COVERAGE)
test-coverage: clean tests
	@echo "Generating coverage report..."
	@gcov $(TEST_SRCS)
	@lcov --capture --directory . --output-file coverage.info
	@genhtml coverage.info --output-directory coverage

# Run specific test
test-%: $(TEST_BUILD_DIR)/%
	./$<

# Installation targets
install: release
	@echo "Installing to $(PREFIX)..."
	@install -d $(PREFIX)/bin
	@install -m 755 $(MAIN) $(PREFIX)/bin/menu_system

uninstall:
	@echo "Uninstalling from $(PREFIX)..."
	@rm -f $(PREFIX)/bin/menu_system

# Clean targets
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.gcov *.gcda *.gcno coverage.info
	@rm -rf coverage

distclean: clean
	@rm -f $(DEPS) $(TEST_DEPS)

# Include generated dependencies
-include $(DEPS) $(TEST_DEPS)

.DEFAULT_GOAL := all

# Help target
help:
	@echo "Menu Extension System v$(VERSION)"
	@echo
	@echo "Available targets:"
	@echo "  all           - Build main binary (default)"
	@echo "  debug         - Build with debug flags"
	@echo "  release       - Build with optimization"
	@echo "  install       - Install to system (default: $(PREFIX))"
	@echo "  uninstall     - Remove from system"
	@echo "  tests         - Build and run all tests"
	@echo "  test-NAME     - Build and run specific test"
	@echo "  test-sanitize - Run tests with sanitizers"
	@echo "  test-valgrind - Run tests with valgrind"
	@echo "  test-memcheck - Run detailed memory checks"
	@echo "  test-coverage - Generate test coverage report"
	@echo "  clean         - Remove build files"
	@echo "  distclean     - Remove build files and dependencies"
	@echo
	@echo "Configuration:"
	@echo "  CC      = $(CC)"
	@echo "  CFLAGS  = $(CFLAGS)"
	@echo "  PREFIX  = $(PREFIX)"
