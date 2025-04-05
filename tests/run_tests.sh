#!/usr/bin/env bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# --- Helper Functions ---
log_info() {
  echo -e "${YELLOW}$1${NC}"
}

log_success() {
  echo -e "${GREEN}$1${NC}"
}

log_error() {
  echo -e "${RED}$1${NC}"
}

# --- Test Execution Functions ---

# Runs standard test binaries
execute_binaries() {
  local test_bins=("$@")
  local overall_status=0

  log_info "Executing test binaries..."
  if [[ ${#test_bins[@]} -eq 0 ]]; then
    log_error "No test binaries provided to execute."
    return 1
  fi

  for test_bin in "${test_bins[@]}"; do
    log_info "--- Running ${test_bin} ---"
    if "${test_bin}"; then
      log_success "--- ${test_bin} PASSED ---"
    else
      log_error "--- ${test_bin} FAILED ---"
      overall_status=1 # Mark overall status as failed
    fi
  done
  return $overall_status
}

# Runs tests with Valgrind
execute_valgrind() {
  local test_bins=("$@")
  local overall_status=0
  # Ensure valgrind exits with non-zero status on error
  local valgrind_cmd="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1"

  log_info "Executing tests with Valgrind..."
  if [[ ${#test_bins[@]} -eq 0 ]]; then
    log_error "No test binaries provided for Valgrind."
    return 1
  fi

  for test_bin in "${test_bins[@]}"; do
    log_info "--- Running Valgrind on ${test_bin} ---"
    if ${valgrind_cmd} "${test_bin}"; then
      log_success "--- Valgrind PASSED for ${test_bin} ---"
    else
      log_error "--- Valgrind FAILED for ${test_bin} ---"
      overall_status=1
    fi
  done
  return $overall_status
}

# Runs tests with Valgrind Memcheck
execute_memcheck() {
  local test_bins=("$@")
  local overall_status=0
  # Ensure memcheck exits with non-zero status on error
  local memcheck_cmd="valgrind --tool=memcheck --leak-check=full --show-reachable=yes --error-exitcode=1"

  log_info "Executing tests with Valgrind Memcheck..."
  if [[ ${#test_bins[@]} -eq 0 ]]; then
    log_error "No test binaries provided for Memcheck."
    return 1
  fi

  for test_bin in "${test_bins[@]}"; do
    log_info "--- Running Memcheck on ${test_bin} ---"
    if ${memcheck_cmd} "${test_bin}"; then
      log_success "--- Memcheck PASSED for ${test_bin} ---"
    else
      log_error "--- Memcheck FAILED for ${test_bin} ---"
      overall_status=1
    fi
  done
  return $overall_status
}

# --- Main Execution Logic ---
main() {
  # Check if any arguments were provided
  if [[ $# -eq 0 ]]; then
    log_error "Usage: $0 [run-binaries|run-valgrind|run-memcheck] [test_binary1 test_binary2 ...]"
    exit 1
  fi

  # Check if we are already running under xvfb-run
  if echo "${XAUTHORITY}" | grep -o '/tmp/xvfb-run.*' >/dev/null 2>&1; then
    # Extract the PID of the xvfb-run process
    #   XVFB_RUN_PID=$(echo "${XAUTHORITY}" | grep -oP '(?<=/tmp/xvfb-run\.)[0-9]+')
    # fi

    # # XAUTHORITY=/tmp/xvfb-run.m4VJMV/Xauthority.Xu9GSu
    # if [[ -z "$XVFB_RUN_PID" ]]; then
    # Not running under xvfb-run, so re-execute the script with it
    log_info "Running inside xvfb-run (PID: $XVFB_RUN_PID, Display: $DISPLAY)"
  else
    # Already running under xvfb-run, proceed with test execution
    log_info "Starting xvfb-run..."
    # Use exec to replace the current process, preserving the exit code
    # Pass necessary arguments to the underlying Xvfb server (using 24-bit depth)
    exec xvfb-run --auto-servernum --server-args="-screen 0 1024x768x24 -ac -nolisten tcp" "$0" "$@"
    # exec should not return here, but add an error just in case
    log_error "Failed to exec xvfb-run"
    exit 1
  fi

  local command="$1"
  shift # Remove command from arguments, leaving the rest (e.g., test binaries)

  local exit_code=0
  case "$command" in
  run-binaries)
    execute_binaries "$@"
    exit_code=$?
    ;;
  run-valgrind)
    execute_valgrind "$@"
    exit_code=$?
    ;;
  run-memcheck)
    execute_memcheck "$@"
    exit_code=$?
    ;;
  *)
    log_error "Unknown command: ${command}"
    log_error "Usage: $0 [run-binaries|run-valgrind|run-memcheck] [test_binary1 test_binary2 ...]"
    exit_code=1
    ;;
  esac

  # xvfb-run handles cleanup automatically when the wrapped command exits
  log_info "Test execution finished with exit code ${exit_code}."
  exit $exit_code
}

# Script Entry Point
main "$@"
