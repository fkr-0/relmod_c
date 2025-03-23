#include "../src/example_menu.h"
#include "../src/input_handler.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

void test_input_handler_handle_key_fn() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(conn);

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  assert(screen);

  xcb_ewmh_connection_t ewmh;
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  assert(xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL));

  InputHandler *handler = input_handler_create(conn, &ewmh, screen->root);
  assert(handler);

  MenuConfig menu_config = example_menu_create(0x40); // Super key

  // Correctly pass the function pointer
  ExampleMenuItem *item = example_menu_add_item(&menu_config, "Test Item",
                                                example_menu_item_command, 0);

  assert(item);

  input_handler_destroy(handler);
  xcb_ewmh_connection_wipe(&ewmh);
  xcb_disconnect(conn);
}

void test_activation_state() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(conn);

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  assert(screen);

  xcb_ewmh_connection_t ewmh;
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  assert(xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL));

  InputHandler *handler = input_handler_create(conn, &ewmh, screen->root);
  assert(handler);

  MenuConfig config = example_menu_create(0x40); // Super key
  ActivationState state = {.config = &config,
                           .mod_key = 0x40,
                           .keycode = 31,
                           .initialized = false,
                           .menu = NULL};
  assert(input_handler_add_activation_state(handler, &state));

  // Simulate key press to activate menu
  xcb_key_press_event_t event = {
      .response_type = XCB_KEY_PRESS, .detail = 31, .state = 0x40};
  assert(input_handler_handle_activation(handler, event.state, event.detail));

  input_handler_destroy(handler);
  xcb_ewmh_connection_wipe(&ewmh);
  xcb_disconnect(conn);
}

int main() {
  test_input_handler_handle_key_fn();
  test_activation_state();
  printf("All tests passed.\n");
  return 0;
}
