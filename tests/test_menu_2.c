/* test_menu.c - Unit tests for menu.c */
#include "../src/menu.h"
#include "../src/menu_defaults.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int activation_called = 0;
static void test_action(void *data) { activation_called++; }

static void test_menu_creation() {
  MenuConfig config = menu_config_default();
  Menu *menu = menu_create(&config);
  assert(menu != NULL);
  assert(menu->state == MENU_STATE_INACTIVE);
  assert(menu->selected_index == 0);
  menu_destroy(menu);
}

static void test_menu_item_selection() {
  MenuItem items[] = {
      {.id = "1", .label = "One", .action = NULL},
      {.id = "2", .label = "Two", .action = NULL},
      {.id = "3", .label = "Three", .action = NULL},
  };

  MenuConfig config = menu_config_default();
  config.items = items;
  config.item_count = 3;
  Menu *menu = menu_create(&config);

  menu_show(menu);
  assert(menu->selected_index == 0);

  menu_select_next(menu);
  assert(menu->selected_index == 1);

  menu_select_prev(menu);
  assert(menu->selected_index == 0);

  menu_select_index(menu, 2);
  assert(menu->selected_index == 2);

  menu_destroy(menu);
}

static void test_direct_key_activation() {
  uint8_t keys[] = {10, 11};
  MenuItem items[] = {
      {.id = "a", .label = "A", .action = test_action},
      {.id = "b", .label = "B", .action = test_action},
  };

  MenuConfig config = menu_config_default();
  config.items = items;
  config.item_count = 2;
  config.nav.direct.keys = keys;
  config.nav.direct.count = 2;

  Menu *menu = menu_create(&config);
  menu_show(menu);

  xcb_key_press_event_t event = {.detail = 11, .state = 0};
  assert(menu_handle_key_press(menu, &event) == false);
  assert(menu->selected_index == 1);

  menu_destroy(menu);
}

int main() {
  test_menu_creation();
  test_menu_item_selection();
  test_direct_key_activation();
  printf("All tests passed.\n");
  return 0;
}
