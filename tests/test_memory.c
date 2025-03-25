/* test_memory.c - Memory profiling and leak detection */
#include "../src/cairo_menu.h"
#include "../src/input_handler.h"
#include "../src/menu_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

/* Memory tracking */
typedef struct {
  size_t allocs;  // Number of allocations
  size_t frees;   // Number of frees
  size_t peak;    // Peak memory usage
  size_t current; // Current memory usage
} MemStats;

static MemStats mem_stats = {0};

/* Memory tracking hooks */
static void *tracked_malloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr) {
    mem_stats.allocs++;
    mem_stats.current += size;
    if (mem_stats.current > mem_stats.peak) {
      mem_stats.peak = mem_stats.current;
    }
  }
  return ptr;
}

static void tracked_free(void *ptr) {
  if (ptr) {
    mem_stats.frees++;
    // Note: Can't track exact size freed without additional bookkeeping
    free(ptr);
  }
}

/* Get current process memory usage */
static size_t get_process_memory(void) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return (size_t)usage.ru_maxrss;
}

/* Reset memory statistics */
static void reset_mem_stats(void) { memset(&mem_stats, 0, sizeof(MemStats)); }

/* Print memory statistics */
static void print_mem_stats(const char *test_name) {
  printf("\nMemory Statistics for %s:\n", test_name);
  printf("Allocations:    %zu\n", mem_stats.allocs);
  printf("Frees:         %zu\n", mem_stats.frees);
  printf("Peak Usage:    %zu bytes\n", mem_stats.peak);
  printf("Current Usage: %zu bytes\n", mem_stats.current);
  printf("Process RSS:   %zu KB\n", get_process_memory());
  printf("Possible Leaks: %zu\n\n", mem_stats.allocs - mem_stats.frees);
}

/* Test menu creation/destruction memory patterns */
static void test_menu_memory(void) {
  printf("Testing menu memory patterns...\n");
  reset_mem_stats();

  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(!xcb_connection_has_error(conn));

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t root = screen->root;

  /* Test menu item creation */
  size_t initial_mem = get_process_memory();

  for (int i = 0; i < 100; i++) {
    MenuItem items[] = {{.id = "test1", .label = "Test 1", .action = NULL},
                        {.id = "test2", .label = "Test 2", .action = NULL}};

    MenuConfig config = {.mod_key = XCB_MOD_MASK_4,
                         .trigger_key = 31,
                         .title = "Memory Test Menu",
                         .items = items,
                         .item_count = 2};

    menu_setup_cairo(conn, root, &config);
    assert(menu != NULL);
    menu_destroy(menu);
  }

  size_t final_mem = get_process_memory();
  print_mem_stats("Menu Creation/Destruction");

  /* Check for memory growth */
  assert(final_mem - initial_mem < 1024); /* Allow small overhead */

  xcb_disconnect(conn);
}

/* Test memory usage during menu operations */
static void test_menu_operations_memory(void) {
  printf("Testing menu operations memory...\n");
  reset_mem_stats();

  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(!xcb_connection_has_error(conn));

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t root = screen->root;

  /* Create test menu */
  MenuItem items[] = {{.id = "test1", .label = "Test 1", .action = NULL},
                      {.id = "test2", .label = "Test 2", .action = NULL}};

  MenuConfig config = {.mod_key = XCB_MOD_MASK_4,
                       .trigger_key = 31,
                       .title = "Memory Test Menu",
                       .items = items,
                       .item_count = 2};

  menu_setup_cairo(conn, root, &config);
  assert(menu != NULL);

  /* Test memory usage during operations */
  size_t initial_mem = get_process_memory();

  for (int i = 0; i < 1000; i++) {
    menu_show(menu);
    menu_select_next(menu);
    menu_select_prev(menu);
    menu_hide(menu);
  }

  size_t final_mem = get_process_memory();
  print_mem_stats("Menu Operations");

  /* Check for memory stability */
  assert(final_mem - initial_mem < 1024); /* Allow small overhead */

  menu_destroy(menu);
  xcb_disconnect(conn);
}

/* Test memory usage with multiple menus */
static void test_multiple_menus_memory(void) {
  printf("Testing multiple menus memory...\n");
  reset_mem_stats();

  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(!xcb_connection_has_error(conn));

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t root = screen->root;

  /* Create menu manager */
  MenuManager *manager = menu_manager_create(conn, NULL);
  assert(manager != NULL);

  size_t initial_mem = get_process_memory();

  /* Create and register multiple menus */
  for (int i = 0; i < 10; i++) {
    MenuItem items[] = {{.id = "test1", .label = "Test 1", .action = NULL},
                        {.id = "test2", .label = "Test 2", .action = NULL}};

    MenuConfig config = {.mod_key = XCB_MOD_MASK_4,
                         .trigger_key = 31 + i,
                         .title = "Memory Test Menu",
                         .items = items,
                         .item_count = 2};

    menu_setup_cairo(conn, root, &config);
    assert(menu != NULL);
    menu_manager_register(manager, menu);
  }

  /* Activate/deactivate menus */
  for (int i = 0; i < 100; i++) {
    MenuRegistryEntry *entry = manager->registry;
    while (entry) {
      menu_manager_activate(manager, entry->menu);
      menu_manager_deactivate(manager);
      entry = entry->next;
    }
  }

  size_t final_mem = get_process_memory();
  print_mem_stats("Multiple Menus");

  /* Check for memory stability */
  assert(final_mem - initial_mem < 2048); /* Allow reasonable overhead */

  menu_manager_destroy(manager);
  xcb_disconnect(conn);
}

/* Run all memory tests */
int main(void) {
  printf("Running Memory Tests\n");
  printf("===================\n\n");

  test_menu_memory();
  test_menu_operations_memory();
  test_multiple_menus_memory();

  printf("\nAll memory tests passed!\n");
  return 0;
}
