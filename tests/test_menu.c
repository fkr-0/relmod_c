/* test_menu.c - Unit tests for menu functionality */
#include "../src/menu.h"
#include <assert.h>
#include <unistd.h> // For usleep

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock X11 environment */
typedef struct {
  xcb_connection_t *conn;
  xcb_window_t root;
  xcb_screen_t *screen;
  X11FocusContext *focus_ctx;
} MockX11;

/* Test action callback counter */
static int action_called = 0;

/* Mock action callback */
static void test_action(void *user_data) {
  action_called++;
  (void)user_data; /* Suppress unused parameter warning */
}

/* Initialize mock X11 environment */
static MockX11 setup_mock_x11(void) {
  MockX11 mock = {0};
  mock.conn = xcb_connect(NULL, NULL);

  int retries = 7;
  while ((!mock.conn || xcb_connection_has_error(mock.conn)) && retries-- > 0) {
    if (mock.conn)
      xcb_disconnect(mock.conn); // Disconnect previous failed attempt
    mock.conn = NULL;            // Ensure conn is NULL if connect fails below
    fprintf(stderr,
            "[WARN] Failed to connect to X server, retrying (%d left)...\n",
            retries + 1);
    usleep(500000); // Wait 500ms
    mock.conn = xcb_connect(NULL, NULL);
  }
  assert(!xcb_connection_has_error(mock.conn));
  /* assert(!xcb_connection_has_error(mock.conn)); */

  xcb_screen_t *screen =
      xcb_setup_roots_iterator(xcb_get_setup(mock.conn)).data;
  mock.root = screen->root;
  mock.screen = screen;

  xcb_ewmh_connection_t ewmh = {0};
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(mock.conn, &ewmh);
  assert(xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL));

  mock.focus_ctx = x11_focus_init(mock.conn, screen->root, &ewmh);
  assert(mock.focus_ctx != NULL);

  return mock;
}

/* Clean up mock environment */
static void cleanup_mock_x11(MockX11 *mock) {
  if (mock->focus_ctx)
    x11_focus_cleanup(mock->focus_ctx);
  if (mock->conn)
    xcb_disconnect(mock->conn);
}

/* Test menu creation/destruction */
static void test_menu_lifecycle(void) {
  printf("Testing menu lifecycle...\n");

  MockX11 mock = setup_mock_x11();

  /* Create test menu items */
  MenuItem items[] = {
      {.id = "test1", .label = "Test 1", .action = test_action},
      {.id = "test2", .label = "Test 2", .action = test_action}};

  /* Create menu configuration */
  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 44, /* j key */
      .title = "Test Menu",
      .items = items,
      .item_count = 2,
      .nav = {.next = {.key = 44, .label = "j"},
              .prev = {.key = 45, .label = "k"},
              .direct = {.keys = (uint8_t[]){10, 11}, .count = 2}},
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = true}};

  /* Create menu */
  Menu *menu = menu_create(&config);
  assert(menu != NULL);
  assert(menu->config.item_count == 2);
  assert(strcmp(menu->config.title, "Test Menu") == 0);
  assert(menu->state == MENU_STATE_INACTIVE);
  assert(!menu->active);
  assert(menu->selected_index == 0);

  /* Clean up */
  menu_destroy(menu);
  cleanup_mock_x11(&mock);

  printf("Menu lifecycle test passed\n");
}

/* Test menu navigation */
static void test_menu_navigation(void) {
  printf("Testing menu navigation...\n");

  MockX11 mock = setup_mock_x11();

  /* Create test menu */
  MenuItem items[] = {{.id = "1", .label = "Item 1", .action = test_action},
                      {.id = "2", .label = "Item 2", .action = test_action},
                      {.id = "3", .label = "Item 3", .action = test_action}};

  MenuConfig config = {
      .title = "Nav Test",
      .items = items,
      .item_count = 3,
      .nav = {.next = {.key = 44, .label = "j"}, // Default nav keys
              .prev = {.key = 45, .label = "k"},
              .direct = {.keys = (uint8_t[]){10, 11, 12}, .count = 3}}};

  Menu *menu = menu_create(&config);
  assert(menu != NULL);

  /* Test initial state */
  assert(menu->selected_index == 0);

  /* Test next selection */
  menu->active = true;
  menu_select_next(menu);
  assert(menu->selected_index == 1);
  menu_select_next(menu);
  assert(menu->selected_index == 2);
  menu_select_next(menu);
  assert(menu->selected_index == 0); /* Wrap around */

  /* Test previous selection */
  menu_select_prev(menu);
  assert(menu->selected_index == 2); /* Wrap around */
  menu_select_prev(menu);
  assert(menu->selected_index == 1);

  /* Test direct selection */
  menu_select_index(menu, 0);
  assert(menu->selected_index == 0);

  /* Test bounds checking */
  menu_select_index(menu, -1);
  assert(menu->selected_index == 0); /* Should not change */
  menu_select_index(menu, 3);
  assert(menu->selected_index == 0); /* Should not change */

  /* Clean up */
  menu_destroy(menu);
  cleanup_mock_x11(&mock);
  mock.focus_ctx = NULL; /* Avoid double free */

  printf("Menu navigation test passed\n");
}

/* Test menu activation/deactivation */
static void test_menu_activation(void) {
  printf("Testing menu activation...\n");

  MockX11 mock = setup_mock_x11();

  /* Create test menu */
  MenuItem items[] = {
      {.id = "test", .label = "Test Item", .action = test_action}};

  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 44,
      .title = "Activation Test",
      .items = items,
      .item_count = 1,
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = true}};

  Menu *menu = menu_create(&config);
  assert(menu != NULL);

  /* Test show/hide */
  menu_show(menu);
  assert(menu->active);
  assert(menu->state == MENU_STATE_INITIALIZING);

  menu_hide(menu);
  assert(!menu->active);
  assert(menu->state == MENU_STATE_INACTIVE);

  /* Test action callback */
  action_called = 0;
  menu_show(menu);
  MenuItem *item = menu_get_selected_item(menu);
  assert(item != NULL);
  item->action(item->metadata);
  assert(action_called == 1);

  /* Clean up */
  menu_destroy(menu);
  cleanup_mock_x11(&mock);
  mock.focus_ctx = NULL; /* Avoid double free */

  printf("Menu activation test passed\n");
}

/* Run all tests */
int main(void) {
  printf("Running menu tests...\n\n");

  test_menu_lifecycle();
  test_menu_navigation();
  test_menu_activation();

  printf("\nAll menu tests passed!\n");
  return 0;
}
