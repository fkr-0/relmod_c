CC = gcc
CFLAGS = -Wall -Wextra -std=c11 `pkg-config --cflags xcb check` --coverage -g -O0
LIBS = `pkg-config --libs xcb check`
LDFLAGS = -lX11 -lxcb-ewmh -lxcb -lxcb-keysyms -lxcb-xkb -lxcb-icccm --coverage
SRCS = src/menu.c src/input_handler.c src/example_menu.c src/main.c src/x11_focus.c
TESTS = src/test_menu.c src/test_input_handler.c
OBJS = $(SRCS:.c=.o)

all: test_menu test_input_handler

example_menu.o: $(OBJS)
	$(CC) $(CFLAGS) -c src/example_menu.c

x11_input_example: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_menu: src/test_menu.c src/example_menu.o src/menu.c src/x11_focus.c
	$(CC) $(CFLAGS) -o test_menu src/test_menu.c src/example_menu.o src/menu.c src/x11_focus.c $(LIBS) $(LDFLAGS)

test_input_handler: src/test_input_handler.c src/input_handler.c src/example_menu.o src/menu.c src/x11_focus.c
	$(CC) $(CFLAGS) -o test_input_handler src/test_input_handler.c src/input_handler.c src/example_menu.o src/menu.c src/x11_focus.c $(LIBS) $(LDFLAGS)

run_tests: all
	./test_menu
	./test_input_handler

coverage: run_tests
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated in coverage_report/index.html"

clean:
	rm -f $(OBJS) x11_input_example test_menu test_input_handler
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report

run:
	sleep 1 && echo 3 && sleep 1 && echo 2 && sleep 1 && echo 1 && ./x11_input_example

.PHONY: all run_tests clean coverage
