# Improved Makefile for Menu Extension System
# Incorporates best practices, semantic versioning check, and organized targets.
#
# Semantic versioning is expected in the format "x.y.z" (e.g., 1.2.3).
#
# The Makefile builds the main binary and runs tests, supports debug/release
# configurations, and generates coverage reports.
#
# Usage:
#   make              # Builds the main binary after validating version.
#   make debug        # Builds with debug flags.
#   make release      # Builds with release optimization.
#   make tests        # Builds and runs all tests.
#   make test-coverage  # Generates coverage report.
#   make install      # Installs binary into PREFIX (default: /usr/local).
#   make clean/distclean  # Cleans build artifacts.
#

VERSION := $(shell grep 'define VERSION "' src/version.h | cut -d '"' -f2)


# -- Semantic Versioning Check -----------------------------------------------
validate_version:
	@echo "Validating semantic version: $(VERSION)"
	@if ! echo "$(VERSION)" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$$'; then \
	  echo "ERROR: VERSION $(VERSION) is not in semantic versioning format (x.y.z)"; \
	  exit 1; \
	fi

# -- Configuration -----------------------------------------------------------
PREFIX ?= /usr/local

# Compiler and Flags
CC         := gcc
CFLAGS_BASE:= -std=c11  -Wall # TODO -Wextra -Werror -pedantic   # Base compiler flags
CFLAGS     := $(CFLAGS_BASE)                               # Will be appended with other flags
CPPFLAGS   := -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L
DEBUGFLAGS := -g3 -O0 -DDEBUG
RELEASEFLAGS := -O2 -DNDEBUG
SANITIZE   := -fsanitize=address -fsanitize=undefined
COVERAGE   := --coverage
DEPFLAGS   = -MT $@ -MMD -MP -MF $(BUILD_DIR)/$*.d

# Define a debug-specific macro for menu debugging
CFLAGS += -DMENU_DEBUG

# Include paths and libraries
INCLUDES   := -I/usr/include/cairo -I/usr/include/xcb -Isrc
LIBS       := -lxcb -lxcb-ewmh -lcairo -lX11 -lm -lxcb-icccm -lgcov -lxcb-util

# Directories for sources, tests, and builds
SRC_DIR       := src
TEST_DIR      := tests
BUILD_DIR     := build
TEST_BUILD_DIR:= $(BUILD_DIR)/tests

# Source files and objects
SRCS   := $(wildcard $(SRC_DIR)/*.c)
OBJS   := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS   := $(OBJS:.o=.d)

# Test files and objects
TEST_SRCS  := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS  := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_BINS  := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%, $(TEST_SRCS))
TEST_DEPS  := $(TEST_OBJS:.o=.d)

# Main binary target
MAIN := $(BUILD_DIR)/rel_mod

# Default target: validate version and build main binary
all: validate_version $(MAIN)

# -- Directory Creation ------------------------------------------------------
$(BUILD_DIR) $(TEST_BUILD_DIR):
	@mkdir -p $@

# -- Compilation Rules -------------------------------------------------------
# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# Compile test files
$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# -- Linking Rules -----------------------------------------------------------
# Link main binary
$(MAIN): $(OBJS)
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Build and run individual tests
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)
	@echo "TEST $@"
	@./$@

# -- Formatting Target -------------------------------------------------------
# Format target: run EditorConfig formatting tool (eclint) to fix source files
format:
	@echo "Formatting source code according to .editorconfig..."
	@command -v eclint >/dev/null 2>&1 || { echo "eclint not installed. Please install it (e.g., npm install -g eclint)"; exit 1; }
	@eclint fix

# -- Phony Targets -----------------------------------------------------------
.PHONY: all validate_version tests test-sanitize test-valgrind test-memcheck \
	debug release install uninstall clean distclean help format ci-coverage

# Build and run all tests
tests: $(TEST_BINS)

# Debug build: append debug flags and build
debug: CFLAGS += $(DEBUGFLAGS)
debug: all

# Release build: append release flags and build
release: CFLAGS += $(RELEASEFLAGS)
release: all

# Run tests with sanitizers enabled
test-sanitize: CFLAGS += $(SANITIZE)
test-sanitize: LDFLAGS += $(SANITIZE)
test-sanitize: tests

# Run tests with valgrind
test-valgrind: debug
	@for test in $(TEST_BINS); do \
	  echo "VALGRIND $$test"; \
	  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 $$test; \
	done

# Run tests with detailed memory checking via memcheck
test-memcheck: debug
	@for test in $(TEST_BINS); do \
	  echo "MEMCHECK $$test"; \
	  valgrind --tool=memcheck --leak-check=full --show-reachable=yes $$test; \
	done

# Generate test coverage report
test-coverage: CFLAGS += $(COVERAGE)
test-coverage: LDFLAGS += $(COVERAGE)
test-coverage: clean tests
	@echo "Running tests to generate coverage data..."
	@for test in $(TEST_BINS); do \
	  echo "Running $$test"; \
	  ./$$test || exit 1; \
	done
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@lcov --capture --directory $(BUILD_DIR) --output-file ./coverage/coverage.info --gcov-tool gcov
	@genhtml ./coverage/coverage.info --output-directory coverage
	@echo "Coverage report generated in coverage/index.html"

# Run a specific test by name (e.g., make test-mytest)
test-%: $(TEST_BUILD_DIR)/%
	./$<

# Calculate and output the overall coverage percentage (useful in CI)
ci-coverage: test-coverage
	@echo "Calculating coverage percentage..."
	@lcov --summary coverage/coverage.info | grep -Eo 'lines\.*: [0-9]+\.[0-9]+%' | awk '{print $$2}'

# Installation: build release and install binary into PREFIX/bin
install: release
	@echo "Installing to $(PREFIX)..."
	@install -d $(PREFIX)/bin
	@install -m 755 $(MAIN) $(PREFIX)/bin/menu_system

# Uninstallation: remove installed binary
uninstall:
	@echo "Uninstalling from $(PREFIX)..."
	@rm -f $(PREFIX)/bin/menu_system

# Clean build artifacts and coverage files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) *.gcov *.gcda *.gcno coverage.info coverage

# distclean: clean and remove dependency files
distclean: clean
	@rm -f $(DEPS) $(TEST_DEPS)

# Include dependency files if available
-include $(DEPS) $(TEST_DEPS)

# Help target: display available targets and configuration details
help:
	@echo "Menu Extension System v$(VERSION)"
	@echo "Available targets:"
	@echo "  all           - Build main binary (default)"
	@echo "  debug         - Build with debug flags"
	@echo "  release       - Build with optimization"
	@echo "  install       - Install to system (default: $(PREFIX))"
	@echo "  uninstall     - Remove from system"
	@echo "  tests         - Build and run all tests"
	@echo "  test-<NAME>   - Build and run a specific test"
	@echo "  test-sanitize - Run tests with sanitizers"
	@echo "  test-valgrind - Run tests with valgrind"
	@echo "  test-memcheck - Run detailed memory checks"
	@echo "  test-coverage - Generate test coverage report"
	@echo "  format        - Format source files according to .editorconfig using eclint"
	@echo "  clean         - Remove build files"
	@echo "  distclean     - Remove build files and dependencies"
	@echo "Configuration:"
	@echo "  CC      = $(CC)"
	@echo "  CFLAGS  = $(CFLAGS)"
	@echo "  PREFIX  = $(PREFIX)"

# Set the default goal to "all"
.DEFAULT_GOAL := all
