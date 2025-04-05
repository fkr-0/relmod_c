/* test_performance.c - Performance benchmarks for menu system */
#include "../src/cairo_menu.h"
#include "../src/input_handler.h"
#include "../src/menu.h"
#include "../src/menu_manager.h"
#include <X11/keysym.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Benchmark configuration */
#define BENCH_ITERATIONS 1000
#define MENU_ITEMS 100
#define WARMUP_ITERATIONS 10

/* Timer utilities */
typedef struct {
  struct timeval start;
  struct timeval end;
} Timer;

static void timer_start(Timer *timer) { gettimeofday(&timer->start, NULL); }

static double timer_end(Timer *timer) {
  gettimeofday(&timer->end, NULL);
  return (timer->end.tv_sec - timer->start.tv_sec) * 1000.0 +
         (timer->end.tv_usec - timer->start.tv_usec) / 1000.0;
}

/* Mock X11 environment */
typedef struct {
  xcb_connection_t *conn;
  xcb_window_t root;
  xcb_screen_t *screen;
  xcb_ewmh_connection_t ewmh;
} MockX11;

/* Initialize mock X11 environment */
static MockX11 setup_mock_x11(void) {
  MockX11 mock = {0};
  mock.conn = xcb_connect(NULL, NULL);
  assert(!xcb_connection_has_error(mock.conn));

  mock.screen = xcb_setup_roots_iterator(xcb_get_setup(mock.conn)).data;
  mock.root = mock.screen->root;

  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(mock.conn, &mock.ewmh);
  assert(xcb_ewmh_init_atoms_replies(&mock.ewmh, cookie, NULL));

  return mock;
}

/* Clean up mock environment */
static void cleanup_mock_x11(MockX11 *mock) {
  xcb_ewmh_connection_wipe(&mock->ewmh);
  xcb_disconnect(mock->conn);
}

/* Create large test menu */
static Menu *create_benchmark_menu(MockX11 *mock) {
  /* Create many menu items */
  MenuItem *items = calloc(MENU_ITEMS, sizeof(MenuItem));
  for (int i = 0; i < MENU_ITEMS; i++) {
    char *label = malloc(32);
    snprintf(label, 32, "Menu Item %d", i + 1);
    items[i] = (MenuItem){
        .id = label, .label = label, .action = NULL, .metadata = NULL};
  }

  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 31,
      .title = "Benchmark Menu",
      .items = items,
      .item_count = MENU_ITEMS,
      .nav = {.next = {.key = 44, .label = "j"},
              .prev = {.key = 45, .label = "k"},
              .direct = {.keys = (uint8_t[]){10, 11, 12, 13}, .count = 4}}};

  InputHandler *handler = input_handler_create();
  Menu *menu = menu_create(&config);
  input_handler_add_menu(handler, menu);

  /* Clean up item labels */
  for (int i = 0; i < MENU_ITEMS; i++) {
    free((char *)items[i].id);
  }
  free(items);

  return menu;
}

/* Benchmark menu creation/destruction */
static void benchmark_menu_lifecycle(MockX11 *mock) {
  printf("Benchmarking menu lifecycle...\n");

  Timer timer;
  double total_time = 0.0;

  /* Warm up */
  for (int i = 0; i < WARMUP_ITERATIONS; i++) {
    Menu *menu = create_benchmark_menu(mock);
    menu_destroy(menu);
  }

  /* Main benchmark */
  for (int i = 0; i < BENCH_ITERATIONS; i++) {
    timer_start(&timer);
    Menu *menu = create_benchmark_menu(mock);
    menu_destroy(menu);
    total_time += timer_end(&timer);
  }

  double avg_time = total_time / BENCH_ITERATIONS;
  printf("Average menu lifecycle time: %.3f ms\n", avg_time);
}

/* Benchmark menu navigation */
static void benchmark_menu_navigation(MockX11 *mock) {
  printf("Benchmarking menu navigation...\n");

  Menu *menu = create_benchmark_menu(mock);
  Timer timer;
  double total_time = 0.0;

  /* Warm up */
  for (int i = 0; i < WARMUP_ITERATIONS; i++) {
    menu_select_next(menu);
  }

  /* Benchmark next selection */
  for (int i = 0; i < BENCH_ITERATIONS; i++) {
    timer_start(&timer);
    menu_select_next(menu);
    total_time += timer_end(&timer);
  }

  double avg_next_time = total_time / BENCH_ITERATIONS;
  printf("Average next selection time: %.3f ms\n", avg_next_time);

  /* Benchmark prev selection */
  total_time = 0.0;
  for (int i = 0; i < BENCH_ITERATIONS; i++) {
    timer_start(&timer);
    menu_select_prev(menu);
    total_time += timer_end(&timer);
  }

  double avg_prev_time = total_time / BENCH_ITERATIONS;
  printf("Average prev selection time: %.3f ms\n", avg_prev_time);

  menu_destroy(menu);
}

/* Benchmark input handling */
static void benchmark_input_handling(MockX11 *mock) {
  printf("Benchmarking input handling...\n");

  InputHandler *handler = input_handler_create();
  Menu *menu = create_benchmark_menu(mock);
  menu_manager_register(handler->menu_manager, menu);

  Timer timer;
  double total_time = 0.0;

  /* Create test key event */
  xcb_key_press_event_t event = {.response_type = XCB_KEY_PRESS,
                                 .detail = 44, /* j key */
                                 .state = XCB_MOD_MASK_4};

  /* Warm up */
  for (int i = 0; i < WARMUP_ITERATIONS; i++) {
    input_handler_handle_event(handler, (xcb_generic_event_t *)&event);
  }

  /* Main benchmark */
  for (int i = 0; i < BENCH_ITERATIONS; i++) {
    timer_start(&timer);
    input_handler_handle_event(handler, (xcb_generic_event_t *)&event);
    total_time += timer_end(&timer);
  }

  double avg_time = total_time / BENCH_ITERATIONS;
  printf("Average input handling time: %.3f ms\n", avg_time);

  input_handler_destroy(handler);
}

/* Run all benchmarks */
int main(void) {
  printf("\nRunning Performance Benchmarks\n");
  printf("=============================\n\n");

  MockX11 mock = setup_mock_x11();

  benchmark_menu_lifecycle(&mock);
  printf("\n");

  benchmark_menu_navigation(&mock);
  printf("\n");

  benchmark_input_handling(&mock);
  printf("\n");

  cleanup_mock_x11(&mock);
  return 0;
}
