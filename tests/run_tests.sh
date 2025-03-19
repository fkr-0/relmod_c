#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configurations
NORMAL=0
SANITIZE=1
COVERAGE=2

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Display header
print_header() {
    echo -e "\n${YELLOW}============================================="
    echo "Running Menu System Tests"
    echo -e "=============================================\n${NC}"
}

# Run a single test with specified configuration
run_test() {
    local test_name=$1
    local config=$2
    local cmd=""

    case $config in
        $NORMAL)
            cmd="make test-$test_name"
            ;;
        $SANITIZE)
            cmd="make test-$test_name CFLAGS+='-fsanitize=address -fsanitize=undefined'"
            ;;
        $COVERAGE)
            cmd="make test-$test_name CFLAGS+='-fprofile-arcs -ftest-coverage'"
            ;;
    esac

    echo -e "${YELLOW}Running $test_name...${NC}"
    if $cmd > /tmp/test.log 2>&1; then
        echo -e "${GREEN}✓ $test_name passed${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}✗ $test_name failed${NC}"
        echo -e "${RED}Log output:${NC}"
        cat /tmp/test.log
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Clean build environment
clean_env() {
    echo -e "\n${YELLOW}Cleaning build environment...${NC}"
    make clean > /dev/null 2>&1
}

# Setup X virtual framebuffer for tests
setup_xvfb() {
    echo -e "${YELLOW}Setting up virtual X server...${NC}"
    export DISPLAY=:99
    Xvfb $DISPLAY -screen 0 1024x768x16 &
    XVFB_PID=$!
    sleep 1
}

# Cleanup X virtual framebuffer
cleanup_xvfb() {
    if [ ! -z "$XVFB_PID" ]; then
        echo -e "${YELLOW}Cleaning up virtual X server...${NC}"
        kill $XVFB_PID
    fi
}

# Run all tests
run_all_tests() {
    local config=$1
    local config_name=""

    case $config in
        $NORMAL)
            config_name="normal"
            ;;
        $SANITIZE)
            config_name="sanitized"
            ;;
        $COVERAGE)
            config_name="coverage"
            ;;
    esac

    echo -e "\n${YELLOW}Running tests in $config_name mode${NC}"

    # Core component tests
    run_test "menu" $config
    run_test "menu_manager" $config

    # Integration tests
    run_test "integration" $config

    if [ $config -eq $COVERAGE ]; then
        echo -e "\n${YELLOW}Generating coverage report...${NC}"
        gcov tests/*.c src/*.c > /dev/null 2>&1
        # Could add lcov here for HTML report
    fi
}

# Display test summary
print_summary() {
    echo -e "\n${YELLOW}============================================="
    echo "Test Summary"
    echo "============================================="
    echo -e "Total tests:  $TOTAL_TESTS"
    echo -e "Passed:      ${GREEN}$PASSED_TESTS${NC}"
    echo -e "Failed:      ${RED}$FAILED_TESTS${NC}"
    echo -e "=============================================\n${NC}"
}

# Main execution
main() {
    print_header

    # Count total tests (each test runs in 3 configurations)
    TOTAL_TESTS=$(($(ls tests/test_*.c | wc -l) * 3))

    # Setup test environment
    setup_xvfb
    trap cleanup_xvfb EXIT

    # Run tests in different configurations
    clean_env
    run_all_tests $NORMAL

    clean_env
    run_all_tests $SANITIZE

    clean_env
    run_all_tests $COVERAGE

    # Display results
    print_summary

    # Return overall status
    [ $FAILED_TESTS -eq 0 ]
}

main