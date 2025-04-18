#include "../src/input_handler.h"
#include "../src/menu_builder.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

static xcb_visualtype_t *get_root_visual_type(xcb_connection_t *conn) {
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter =
        xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        return visual_iter.data;
      }
    }
  }
  return NULL;
}
void example_menu_item_cmd(void *user_data) {
  printf("Menu item selected: %s\n", (char *)user_data);
}

void test_input_handler_handle_key_fn() {

  InputHandler *handler = input_handler_create();

  assert(input_handler_setup_x(handler) && "Input handler X setup failed");
  // EWMH is initialized within input_handler_setup_x
  xcb_connection_t *conn = handler->conn;
  assert(conn);

  xcb_screen_t *screen = handler->screen;
  assert(screen);

  xcb_visualtype_t *visual = get_root_visual_type(conn);
  assert(visual);
  assert(handler);

  MenuBuilder builder = menu_builder_create("Demo Menu", 3);
  menu_builder_add_item(&builder, "Item 1", example_menu_item_cmd, 0);
  menu_builder_add_item(&builder, "Item 2", example_menu_item_cmd, 0);
  menu_builder_add_item(&builder, "Item 3", example_menu_item_cmd, 0);

  MenuConfig *menu_config = menu_builder_finalize(&builder);
  // Correctly pass the function pointer

  MenuItem *item = &menu_config->items[0];

  assert(item);

  menu_config_destroy(menu_config); // Destroy the config created locally
  // Let input_handler_destroy handle EWMH wipe and connection disconnect
  input_handler_destroy(handler);
  // Cleanup (after exit)
}

void test_activation_state() {
  InputHandler *handler = input_handler_create();

  assert(input_handler_setup_x(handler) && "Input handler X setup failed");
  // EWMH is initialized within input_handler_setup_x
  xcb_connection_t *conn = handler->conn;
  assert(conn);

  xcb_screen_t *screen = handler->screen;
  assert(screen);

  xcb_visualtype_t *visual = get_root_visual_type(conn);
  assert(visual);
  assert(handler);

  MenuBuilder builder = menu_builder_create("Demo Menu", 3);
  menu_builder_add_item(&builder, "Item 1", example_menu_item_cmd, 0);
  menu_builder_add_item(&builder, "Item 2", example_menu_item_cmd, 0);
  menu_builder_add_item(&builder, "Item 3", example_menu_item_cmd, 0);
  menu_builder_set_mod_key(&builder, XCB_MOD_MASK_4);              // Super key
  menu_builder_set_trigger_key(&builder, 31);                      // Space key
  menu_builder_set_activation_state(&builder, XCB_MOD_MASK_4, 31); // Super + i

  MenuConfig *menu_config = menu_builder_finalize(&builder);

  Menu *menu = menu_create(menu_config);
  assert(menu);

  // Simulate key press to activate menu
  xcb_key_press_event_t event = {
      .response_type = XCB_KEY_PRESS, .detail = 31, .state = 0x40};
  input_handler_add_menu(handler, menu);
  assert(input_handler_handle_activation(handler, event.state, event.detail));

  // Let input_handler_destroy handle EWMH wipe and connection disconnect
  // menu_config is managed by the handler now, so don't destroy it here.
  input_handler_destroy(handler);
}

int main() {
  test_input_handler_handle_key_fn();
  test_activation_state();
  printf("All tests passed.\n");
  return 0;
}
