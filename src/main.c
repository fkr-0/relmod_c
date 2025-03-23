// ====================== FILE: main.c (safe minimal example)
// ======================
#include "cairo_menu.h"
#include "input_handler.h"
#include "menu_builder.h"
#include <stdio.h>
#include <xcb/xcb.h>
#ifdef MENU_DEBUG
#define LOG_PREFIX "[MAIN]"
#include "log.h"
#endif

void demo_action(void *user_data) {
  printf("Menu item selected: %s\n", (char *)user_data);
}

// Correct way: only build config, no direct cairo_menu_create() here!
MenuConfig *create_menu_config(void) {
  MenuBuilder builder = menu_builder_create("Demo Menu", 3);
  menu_builder_add_item(&builder, "Item 1", demo_action, "item1");
  menu_builder_add_item(&builder, "Item 2", demo_action, "item2");
  menu_builder_add_item(&builder, "Item 3", demo_action, "item3");

  menu_builder_set_mod_key(&builder, XCB_MOD_MASK_4);              // Super key
  menu_builder_set_trigger_key(&builder, 31);                      // Space key
  menu_builder_set_activation_state(&builder, XCB_MOD_MASK_4, 31); // Super + i
  MenuConfig *config = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return config;
}

int main(void) {
  /* xcb_connection_t *conn = xcb_connect(NULL, NULL); */

  /* xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
   */
  /* xcb_window_t root = screen->root; */

  InputHandler *handler = input_handler_create();
  input_handler_setup_x(handler);
  LOG("handler->connection: %p", handler->conn);
  xcb_connection_t *conn = handler->conn;
  if (xcb_connection_has_error(conn)) {
    fprintf(stderr, "Cannot connect to X server\n");
    return EXIT_FAILURE;
  }

  MenuConfig *config = create_menu_config();

  // log menu style
  printf("Menu style: background_color: %f, %f, %f, %f\n",
         config->style.background_color[0], config->style.background_color[1],
         config->style.background_color[2], config->style.background_color[3]);
  // Crucial fix: Only register CONFIG, NOT menu itself
  input_handler_add_menu(handler, config);

  // Run the input handler event loop
  input_handler_run(handler);

  // Cleanup (after exit)
  menu_config_destroy(config);
  /* input_handler_destroy(handler); */
  xcb_disconnect(conn);
  return EXIT_SUCCESS;
}
