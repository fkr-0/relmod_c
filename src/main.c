/* ======= main.c ======= */
#include "example_menu.h"
#include "input_handler.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

// Example callback for actions (replace with real logic)
bool example_menu_action(uint8_t keycode, void *user_data) {
  printf("Menu action triggered by keycode %d\n", keycode);
  return true; // keep menu active
}

void example_cleanup(void *user_data) { printf("Menu cleaned up\n"); }

int main(void) {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(conn)) {
    fprintf(stderr, "Failed to connect to X server\n");
    return EXIT_FAILURE;
  }

  MenuConfig config = {.modifier_mask = XCB_MOD_MASK_4, // e.g., Super key
                       .action_cb = example_menu_action,
                       .cleanup_cb = example_cleanup,
                       .user_data = NULL};

  xcb_ewmh_connection_t ewmh = {0};
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  xcb_ewmh_init_atoms_replies(&ewmh, cookie, (void *)0);

  xcb_window_t root = xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;

  X11FocusContext *focus_ctx = x11_focus_init(conn, &ewmh);
  MenuContext *menu_ctx = menu_create(focus_ctx, config);
  InputHandler *handler =
      input_handler_create(conn, &ewmh, root); // Initialize input handler

  MenuConfig menu = example_menu_create(XCB_MOD_MASK_4);
  input_handler_add_menu(handler, menu);

  input_handler_run(handler); // Main loop

  input_handler_destroy(handler);
  menu_destroy(menu_ctx);
  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}
