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
# Default install prefix (used if not root)
PREFIX ?= $(HOME)/.local
# Installation directory determined based on user privileges
ifeq ($(shell id -u),0)
  INSTALL_DIR := /usr/bin
else
  INSTALL_DIR := $(PREFIX)/bin
endif

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
BUILD_DIR         := build
TEST_BUILD_DIR    := $(BUILD_DIR)/tests
EXAMPLE_SRC_DIR   := examples
EXAMPLE_BUILD_DIR := $(BUILD_DIR)/examples

# Source files and objects
SRCS   := $(wildcard $(SRC_DIR)/*.c)
OBJS   := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS   := $(OBJS:.o=.d)

# Test files and objects
TEST_SRCS  := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS  := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_BINS  := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%, $(TEST_SRCS))
TEST_DEPS  := $(TEST_OBJS:.o=.d)

# Example files and objects
EXAMPLE_SRCS := $(filter-out $(EXAMPLE_SRC_DIR)/plugin_template.c $(EXAMPLE_SRC_DIR)/plugin_test_template.c, $(wildcard $(EXAMPLE_SRC_DIR)/*.c)) # Exclude templates
EXAMPLE_OBJS := $(patsubst $(EXAMPLE_SRC_DIR)/%.c, $(EXAMPLE_BUILD_DIR)/%.o, $(EXAMPLE_SRCS))
EXAMPLE_BINS := $(patsubst $(EXAMPLE_SRC_DIR)/%.c, $(EXAMPLE_BUILD_DIR)/%, $(EXAMPLE_SRCS))
EXAMPLE_DEPS := $(EXAMPLE_OBJS:.o=.d)

# Main binary target
MAIN := $(BUILD_DIR)/rel_mod

# Default target: validate version and build main binary
all: validate_version $(MAIN)

# -- Directory Creation ------------------------------------------------------
$(BUILD_DIR) $(TEST_BUILD_DIR) $(EXAMPLE_BUILD_DIR):
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

# Compile example files
$(EXAMPLE_BUILD_DIR)/%.o: $(EXAMPLE_SRC_DIR)/%.c | $(EXAMPLE_BUILD_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# -- Linking Rules -----------------------------------------------------------
# Link main binary
$(MAIN): $(OBJS)
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Build individual test binaries (execution handled by run_tests.sh)
$(TEST_BUILD_DIR)/%: $(TEST_BUILD_DIR)/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	@echo "LINK (Test) $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

# Link example binaries (link against all main objects except main.o)
$(EXAMPLE_BUILD_DIR)/%: $(EXAMPLE_BUILD_DIR)/%.o $(filter-out $(BUILD_DIR)/main.o, $(OBJS))
	@echo "LINK $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

# -- Formatting Target -------------------------------------------------------
# Format target: run EditorConfig formatting tool (eclint) to fix source files
format:
	@echo "Formatting source code according to .editorconfig..."
	@command -v eclint >/dev/null 2>&1 || { echo "eclint not installed. Please install it (e.g., npm install -g eclint)"; exit 1; }
	@eclint fix

# -- Phony Targets -----------------------------------------------------------
.PHONY: all validate_version build-tests tests run-test-binaries test-sanitize \
	run-valgrind test-valgrind run-memcheck test-memcheck test-coverage \
	run-test-% test-% \
	debug release examples install uninstall clean distclean help format ci-coverage

# Build all test binaries
build-tests: $(TEST_BINS)

# Run all tests using the script
tests: build-tests run-test-binaries

# Internal target to run standard tests via script
run-test-binaries:
	@echo "Running tests via script..."
	@./tests/run_tests.sh run-binaries $(TEST_BINS)

# Build examples
examples: $(EXAMPLE_BINS)

# Debug build: append debug flags and build
debug: CFLAGS += $(DEBUGFLAGS)
debug: all

# Release build: append release flags and build
release: CFLAGS += $(RELEASEFLAGS)
release: all

# Run tests with sanitizers enabled
# Build and run tests with sanitizers enabled
test-sanitize: CFLAGS += $(SANITIZE)
test-sanitize: LDFLAGS += $(SANITIZE)
test-sanitize: build-tests run-test-binaries # Re-build with sanitize flags and run

# Run tests with valgrind
# Build debug binaries and run with valgrind via script
test-valgrind: debug run-valgrind # Depends on debug build, then runs valgrind

# Internal target to run valgrind via script
run-valgrind:
	@echo "Running tests with Valgrind via script..."
	@./tests/run_tests.sh run-valgrind $(TEST_BINS)

# Run tests with detailed memory checking via memcheck
# Build debug binaries and run with memcheck via script
test-memcheck: debug run-memcheck # Depends on debug build, then runs memcheck

# Internal target to run memcheck via script
run-memcheck:
	@echo "Running tests with Memcheck via script..."
	@./tests/run_tests.sh run-memcheck $(TEST_BINS)

# Generate test coverage report
# Generate test coverage report (builds with coverage, runs via script, generates report)
test-coverage: CFLAGS += $(COVERAGE)
test-coverage: LDFLAGS += $(COVERAGE)
test-coverage: clean build-tests # Clean first, then build with coverage flags
	@echo "Running tests via script to generate coverage data..."
	@./tests/run_tests.sh run-binaries $(TEST_BINS) # Run tests using the script
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@lcov --capture --directory $(BUILD_DIR) --output-file ./coverage/coverage.info --gcov-tool gcov
	@genhtml ./coverage/coverage.info --output-directory coverage
	@echo "Coverage report generated in coverage/index.html"

# Build a specific test by name (e.g., make test-mytest)
test-%: $(TEST_BUILD_DIR)/%
	@echo "Built test binary: $<"

# Run a specific test by name using the script (e.g., make run-test-mytest)
run-test-%: test-% # Depends on building the specific test first
	@echo "Running specific test via script: $(TEST_BUILD_DIR)/$*"
	@./tests/run_tests.sh run-binaries $(TEST_BUILD_DIR)/$*

# Calculate and output the overall coverage percentage (useful in CI)
ci-coverage: test-coverage
	@echo "Calculating coverage percentage..."
	@lcov --summary coverage/coverage.info | grep -Eo 'lines\.*: [0-9]+\.[0-9]+%' | awk '{print $$2}'

# Installation: build release and install binary into PREFIX/bin
install: release
	@echo "Installing to $(INSTALL_DIR)..."
	@install -d $(INSTALL_DIR)
	@install -m 755 $(MAIN) $(INSTALL_DIR)/rel_mod

# Uninstallation: remove installed binary
uninstall:
	@echo "Uninstalling from $(INSTALL_DIR)..."
	@rm -f $(INSTALL_DIR)/rel_mod

# Clean build artifacts and coverage files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) *.gcov *.gcda *.gcno coverage.info coverage $(EXAMPLE_BINS)

# distclean: clean and remove dependency files
distclean: clean
	@rm -f $(DEPS) $(TEST_DEPS) $(EXAMPLE_DEPS)

# Include dependency files if available
-include $(DEPS) $(TEST_DEPS) $(EXAMPLE_DEPS)

# Help target: display available targets and configuration details
help:
	@echo "Menu Extension System v$(VERSION)"
	@echo "Available targets:"
	@echo "  all           - Build main binary (default)"
	@echo "  debug         - Build with debug flags"
	@echo "  release       - Build with optimization"
	@echo "  install       - Install to system ($(INSTALL_DIR))"
	@echo "  examples      - Build example programs"
	@echo "  uninstall     - Remove from system"
	@echo "  tests         - Build and run all tests (using tests/run_tests.sh)"
	@echo "  test-<NAME>   - Build a specific test binary (e.g., make test-mytest)"
	@echo "  run-test-<NAME> - Run a specific test binary via script (e.g., make run-test-mytest)"
	@echo "  test-sanitize - Build and run tests with sanitizers (using tests/run_tests.sh)"
	@echo "  test-valgrind - Build debug and run tests with valgrind (using tests/run_tests.sh)"
	@echo "  test-memcheck - Build debug and run detailed memory checks (using tests/run_tests.sh)"
	@echo "  test-coverage - Generate test coverage report (using tests/run_tests.sh)"
	@echo "  format        - Format source files according to .editorconfig using eclint"
	@echo "  clean         - Remove build files"
	@echo "  distclean     - Remove build files and dependencies"
	@echo "Configuration:"
	@echo "  CC      = $(CC)"
	@echo "  CFLAGS  = $(CFLAGS)"
	@echo "  INSTALL_DIR = $(INSTALL_DIR)"

# Set the default goal to "all"
.DEFAULT_GOAL := all
