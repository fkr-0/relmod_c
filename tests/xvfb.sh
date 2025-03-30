#/usr/bin/env bash

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

# Default display
export DISPLAY=:99

setup_xvfb() {
  echo -e "${YELLOW}Setting up virtual X server on $DISPLAY...${NC}"

  # Check if already running
  if pgrep -f "Xvfb $DISPLAY" >/dev/null; then
    echo -e "${YELLOW}Xvfb already running on $DISPLAY${NC}"
    return 0
  fi

  # Start Xvfb with tighter error handling
  sudo Xvfb $DISPLAY -screen 0 1024x768x16 -ac -nolisten tcp &
  XVFB_PID=$!

  # Wait for Xvfb to be ready
  local max_retries=5
  local retry_count=0

  while [ $retry_count -lt $max_retries ]; do
    xdpyinfo -display $DISPLAY >/dev/null 2>&1
    if [ $? -eq 0 ]; then
      echo -e "${GREEN}Xvfb started successfully on $DISPLAY${NC}"
      export XVFB_PID=$XVFB_PID
      return 0
    fi

    sleep 1
    ((retry_count++))
  done

  echo -e "${RED}Failed to start Xvfb after $max_retries attempts${NC}"
  return 1
}

cleanup_xvfb() {
  echo -e "${YELLOW}Cleaning up virtual X server...${NC}"
  if [ -n "$XVFB_PID" ]; then
    if kill -0 $XVFB_PID >/dev/null 2>&1; then
      kill $XVFB_PID
      echo -e "${GREEN}Stopped Xvfb (PID $XVFB_PID)${NC}"
    else
      echo -e "${YELLOW}Xvfb (PID $XVFB_PID) was not running${NC}"
    fi
  fi
  unset XVFB_PID
}

usage() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -h, --help      Show this help message and exit"
  echo "  -s, --setup     Setup X virtual framebuffer"
  echo "  -c, --cleanup   Cleanup X virtual framebuffer"
}

# Main execution
case "$1" in
-s | -setup) setup_xvfb ;;
-c | --cleanup) cleanup_xvfb ;;
-h | --help) usage ;;
*)
  usage >&2
  exit 1
  ;;
esac
