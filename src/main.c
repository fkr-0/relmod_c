// ====================== FILE: main.c (safe minimal example) ==================
#include "input_handler.h"
#include "key_helper.h"
#include "menu_builder.h"
#include <stdio.h>
#include <xcb/xcb.h>
#ifdef MENU_DEBUG
#define LOG_PREFIX "[MAIN]"
#endif
#include "log.h"

void demo_action(void *user_data) {
  printf("Menu item selected: %s\n", (char *)user_data);
}

// Correct way: only build config, no direct menu_setup_cairo() here!
MenuConfig *create_menu_config(void) {
  MenuBuilder builder = menu_builder_create("Demo Menu", 3);
  menu_builder_add_item(&builder, "Item 1", demo_action, "item1");
  menu_builder_add_item(&builder, "Item 2", demo_action, "item2");
  menu_builder_add_item(&builder, "Item 3", demo_action, "item3");

  menu_builder_set_mod_key(&builder, XCB_MOD_MASK_4); // Super key
  menu_builder_set_trigger_key(&builder, 31);         // Space key
  /* menu_builder_set_navigation_keys(MenuBuilder *builder, uint8_t next_key,
   * const char *next_label, uint8_t prev_key, const char *prev_label, uint8_t
   * *direct_keys, size_t direct_count) */
  menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL, 0); // j, k

  menu_builder_set_activation_state(&builder, XCB_MOD_MASK_4, 31); // Super + i
  MenuConfig *config = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return config;
}

int main(int argc, char *argv[]) {
  printf("argc: %d\n", argc);
  printf("argv[0]: %s\n", argv[0]);

  uint8_t keycode = 0;
  if (argc == 2) {
    keycode = atoi(argv[1]);
  }
  if (argc > 2) {
    printf("Usage: %s <KEYCODE>\n", argv[0]);
    return EXIT_FAILURE;
  }

  InputHandler *handler = input_handler_create();
  input_handler_setup_x(handler);
  LOG("handler->connection: %p", handler->conn);

  MenuConfig *config = create_menu_config();

  // log menu style
  printf("Menu style: background_color: %f, %f, %f, %f\n",
         config->style.background_color[0], config->style.background_color[1],
         config->style.background_color[2], config->style.background_color[3]);
  // Crucial fix: Only register CONFIG, NOT menu itself
  input_handler_add_menu(handler, config);

  xcb_connection_t *conn = handler->conn;
  if (xcb_connection_has_error(conn)) {
    fprintf(stderr, "Cannot connect to X server\n");
    return EXIT_FAILURE;
  }
  if (keycode > 0) {
    uint16_t state = mod_state(handler->conn);
    xcb_generic_event_t event = key_press(keycode, state);
    input_handler_handle_event(handler, &event);
    event = key_release(keycode, state);
    LOG("injecting release code: %d state: %d", keycode, state);
    input_handler_handle_event(handler, &event);
  }
  // Run the input handler event loop
  input_handler_run(handler);

  // Cleanup (after exit)
  menu_config_destroy(config);
  /* input_handler_destroy(handler); */
  xcb_disconnect(conn);
  return EXIT_SUCCESS;
}
