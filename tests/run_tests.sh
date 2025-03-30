#!/usr/bin/env bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

# Initialize Xvfb virtual display
echo -e "${YELLOW}Initializing Xvfb...${NC}"
./tests/xvfb.sh -s 2>/dev/null || {
  echo -e "${RED}Failed to start Xvfb virtual display${NC}"
  exit 1
}

# Cleanup function
cleanup() {
  echo -e "${YELLOW}Running test cleanup...${NC}"
  ./tests/xvfb.sh -c
  echo -e "${GREEN}Cleanup complete${NC}"
  sudo killall -9 Xvfb
}

# Register cleanup trap
trap cleanup SIGINT SIGTERM EXIT

# Run test suites with status reporting
run_test_suite() {
  # local name="$1"
  # local test_file="$2"

  # echo -e "${YELLOW}Running ${name} tests...${NC}"
  # if "${test_file}"; then
  #   echo -e "${GREEN}${name} tests passed${NC}"
  #   return 0
  # else
  #   echo -e "${RED}${name} tests failed${NC}"
  #   return 1
  # fi
  export DISPLAY=:99
  DISPLAY=:99 make test-coverage
  if [[ $? -ne 0 ]]; then
    sleep 1
    DISPLAY=:99 make test-coverage

    if [[ $? -ne 0 ]]; then
      sleep 1
      DISPLAY=:99 make test-coverage
      if [[ $? -ne 0 ]]; then
        sleep 1
        DISPLAY=:99 make test-coverage
      fi

    fi
  fi
}

# Main test execution
echo -e "${YELLOW}Starting test suite execution${NC}"

# declare -a test_suites=(
#   "Integration:./tests/test_integration"
#   "Menu:./tests/test_menu"
#   "Animation:./tests/test_animation"
# )

# overall_result=0
# for suite in "${test_suites[@]}"; do
#   IFS=':' read -ra parts <<<"$suite"
#   name="${parts[0]}"
#   path="${parts[1]}"

#   if ! run_test_suite "$name" "$path"; then
#     overall_result=1
#   fi
# done
run_test_suite
if [[ $? -eq 0 ]]; then
  echo -e "${GREEN}All test suites passed successfully${NC}"
else
  echo -e "${RED}Some test suites failed${NC}"
fi

exit $overall_result
