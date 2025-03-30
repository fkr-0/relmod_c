#/usr/bin/env bash

setup_xvfb() {
  echo -e "${YELLOW}Setting up virtual X server...${NC}"
  export DISPLAY=:99
  sudo nohup Xvfb $DISPLAY -screen 2 1024x768x16 >/dev/null &
  XVFB_PID=$!
  sleep 2
  pgrep -lfa Xvfb
}

# Cleanup X virtual framebuffer
cleanup_xvfb() {
  if [ ! -z "$XVFB_PID" ]; then
    echo -e "${YELLOW}Cleaning up virtual X server...${NC}"
    sudo kill $XVFB_PID
  else
    echo -e "${YELLOW}No virtual X server PID to cleanup, using pkill${NC}"
    sudo pkill Xvfb
  fi
}

usage() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  -h, --help      Show this help message and exit"
  echo "  -s, --setup     Setup X virtual framebuffer"
  echo "  -c, --cleanup   Cleanup X virtual framebuffer"
}

[ -z "$1" ] && usage && exit 1

# Parse command line arguments
while [ "$1" != "" ]; do
  case $1 in
  -s | --setup)
    setup_xvfb
    ;;
  -c | --cleanup)
    cleanup_xvfb
    ;;
  -h | --help)
    usage
    exit
    ;;
  *)
    usage
    exit 1
    ;;
  esac
  shift
done
