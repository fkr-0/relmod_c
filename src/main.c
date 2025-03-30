// ====================== FILE: main.c (safe minimal example) ==================
#include "input_handler.h"
#include "key_helper.h"
#include "menu_builder.h"
#include "version.h"
#include "window_menu.h"
#include "x11_window.h"
#include <stdio.h>
#include <xcb/xcb.h>
#ifdef MENU_DEBUG
#define LOG_PREFIX "[MAIN]"
#endif
#include "log.h"

void demo_action(void *user_data) {
  printf("Menu item selected: %s\n", (char *)user_data);
}
/* void window_menu_on_select(MenuItem *item, void *user_data) {
 * LOG("OnSelect"); } */

// Correct way: only build config, no direct menu_setup_cairo() here!
MenuConfig *create_menu_config(void) {
  MenuBuilder builder = menu_builder_create("Demo Menu", 3);
  menu_builder_add_item(&builder, "Item 1", demo_action, "item1");
  menu_builder_add_item(&builder, "Item 2", demo_action, "item2");
  menu_builder_add_item(&builder, "Item 3", demo_action, "item3");

  menu_builder_set_mod_key(&builder, XCB_MOD_MASK_4); // Super key
  /* menu_builder_set_trigger_key(&builder, 32);         // Space key */
  /* menu_builder_set_navigation_keys(MenuBuilder *builder, uint8_t next_key,
   * const char *next_label, uint8_t prev_key, const char *prev_label, uint8_t
   * *direct_keys, size_t direct_count) */
  menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL,
                                   0); // j, k

  menu_builder_set_activation_state(&builder, XCB_MOD_MASK_4,
                                    32); // Super + i
  MenuConfig *config = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return config;
}
static MenuConfig rebuild_menu_config(WindowMenu *wm, uint16_t mod_key,
                                      uint8_t trigger_key) {
  MenuBuilder builder =
      menu_builder_create("Window Menu", wm->window_list->count);
  for (size_t i = 0; i < wm->window_list->count; i++) {
    xcb_window_t *win_ptr = malloc(sizeof(xcb_window_t));
    if (!win_ptr) {
      fprintf(stderr, "Failed to allocate memory for window id\n");
      exit(EXIT_FAILURE);
    }
    *win_ptr = wm->window_list->windows[i].id;
    menu_builder_add_item(&builder, wm->window_list->windows[i].title, NULL,
                          win_ptr);
  }

  menu_builder_set_trigger_key(&builder, trigger_key);
  menu_builder_set_mod_key(&builder, mod_key);
  menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL,
                                   0); // j, k
  menu_builder_set_activation(&builder, true, true);
  MenuConfig *config = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return *config;
}
/* static MenuConfig build_menu_config(WindowMenu *wm, uint16_t modifier_mask) {
 */
/*     MenuBuilder builder = */
/*         menu_builder_create("Window Menu", wm->window_list->count); */
/*     for (size_t i = 0; i < wm->window_list->count; i++) { */
/*         xcb_window_t *win_ptr = malloc(sizeof(xcb_window_t)); */
/*         if (!win_ptr) { */
/*             fprintf(stderr, "Failed to allocate memory for window id\n"); */
/*             exit(EXIT_FAILURE); */
/*         } */
/*         *win_ptr = wm->window_list->windows[i].id; */
/*         menu_builder_add_item(&builder, wm->window_list->windows[i].title,
 * NULL, */
/*                               win_ptr); */
/*     } */

/*     menu_builder_set_trigger_key(&builder, 31); */
/*     menu_builder_set_mod_key(&builder, modifier_mask); */
/*     menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL, */
/*                                      0); // j, k */
/*     menu_builder_set_activation(&builder, true, true); */
/*     MenuConfig *config = menu_builder_finalize(&builder); */
/*     menu_builder_destroy(&builder); */
/*     return *config; */
/* } */

/* MenuConfig *create_window_menu_config(void) { */
/*   MenuBuilder builder = menu_builder_create("wmn", 10); */
/*   /\* menu_builder_add_item(&builder, "Item 1", demo_action, "item1"); *\/ */
/*   /\* menu_builder_add_item(&builder, "Item 2", demo_action, "item2"); *\/ */
/*   /\* menu_builder_add_item(&builder, "Item 3", demo_action, "item3"); *\/ */

/*   menu_builder_set_mod_key(&builder, XCB_MOD_MASK_4); // Super key */
/*   menu_builder_set_trigger_key(&builder, 32);         // Space key */
/*   /\* menu_builder_set_navigation_keys(MenuBuilder *builder, uint8_t
 * next_key, */
/*    * const char *next_label, uint8_t prev_key, const char *prev_label,
 * uint8_t */
/*    * *direct_keys, size_t direct_count) *\/ */
/*   menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL, */
/*                                    0); // j, k */

/*   menu_builder_set_activation_state(&builder, XCB_MOD_MASK_4, */
/*                                     49); // Super + i */
/*   MenuConfig *config = menu_builder_finalize(&builder); */
/*   menu_builder_destroy(&builder); */
/*   return config; */
/* } */
int main(int argc, char *argv[]) {
  printf("===== relmod_c v%s =====\n", VERSION);
  printf("argc: %d\n", argc);
  // loop over argv
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]: %s\n", i, argv[i]);
  }

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
  LOG("handler->connection: %p", (void *)handler->conn);

  /* MenuConfig *config = create_menu_config(); */

  /* input_handler_add_menu(handler, config); */

  xcb_connection_t *conn = handler->conn;
  WindowList *window_list = window_list_init(conn);

  SubstringsFilterData sub_data =
      substrings_filter_data((const char *[]){"Chrom", "Firefox"}, 2);
  /* SubstringFilterData sub_data = substring_filter_data("macs"); */
  WindowList *window_list_f =
      window_list_filter(window_list, window_filter_substrings_any, &sub_data);
  WindowMenu *window_menu =
      window_menu_create(conn, window_list_f, SUPER_MASK, 31);
  MenuConfig window_menu_config =
      rebuild_menu_config(window_menu, SUPER_MASK, 31);
  Menu *m = input_handler_add_menu(handler, &window_menu_config);
  m->on_select = window_menu_on_select;

  sub_data = substrings_filter_data((const char *[]){"macs", "Visual"}, 2);
  /* SubstringFilterData sub_data = substring_filter_data("macs"); */
  window_list_f =
      window_list_filter(window_list, window_filter_substrings_any, &sub_data);
  window_menu = window_menu_create(conn, window_list_f, SUPER_MASK, 30);
  window_menu_config = rebuild_menu_config(window_menu, SUPER_MASK, 30);
  m = input_handler_add_menu(handler, &window_menu_config);
  m->on_select = window_menu_on_select;

  sub_data = substrings_filter_data((const char *[]){"tmux", "kitty"}, 2);
  /* SubstringFilterData sub_data = substring_filter_data("macs"); */
  window_list_f =
      window_list_filter(window_list, window_filter_substrings_any, &sub_data);
  window_menu = window_menu_create(conn, window_list_f, SUPER_MASK, 32);
  window_menu_config = rebuild_menu_config(window_menu, SUPER_MASK, 32);
  m = input_handler_add_menu(handler, &window_menu_config);
  m->on_select = window_menu_on_select;

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

  // Initialize window list
  /* WindowList *window_list = window_list_init(conn); */

  /* // Create filtered window menus */
  /* WindowMenuConfig chrom_firefox_menu = window_menu_create_filtered( */
  /*     conn, *handler->root, XCB_MOD_MASK_4, window_list, "chrom"); */
  /* chrom_firefox_menu.modifier_mask = XCB_MOD_MASK_4; */
  /* /\* chrom_firefox_menu.action_cb = window_menu_create; *\/ */

  /* chrom_firefox_menu.cleanup_cb = window_menu_cleanup; */
  /* chrom_firefox_menu.user_data = window_list; */

  /* WindowMenuConfig emacs_menu = window_menu_create_filtered( */
  /*     conn, *handler->root, XCB_MOD_MASK_4, window_list, "emacs"); */
  /* emacs_menu.modifier_mask = XCB_MOD_MASK_4; */
  /* /\* emacs_menu.action_cb = window_menu_action; *\/ */
  /* emacs_menu.cleanup_cb = window_menu_cleanup; */
  /* emacs_menu.user_data = window_list; */

  // Run the input handler event loop
  input_handler_run(handler);

  /* // Run event loop */
  /* while (1) { */
  /*   xcb_generic_event_t *event; */
  /*   while ((event = xcb_poll_for_event(conn))) { */
  /*     if (event->response_type == XCB_KEY_PRESS) { */
  /*       xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event; */
  /*       if (kp->detail == 24) { // 'u' key */
  /*         window_menu_create_filtered(conn, *handler->root, XCB_MOD_MASK_4,
   */
  /*                                     window_list, "chrom"); */
  /*       } else if (kp->detail == 32) { // 'o' key */
  /*         window_menu_create_filtered(conn, *handler->root, XCB_MOD_MASK_4,
   */
  /*                                     window_list, "emacs"); */
  /*       } */
  /*     } */
  /*     free(event); */
  /*   } */
  /* } */

  // Cleanup (after exit)
  /* menu_config_destroy(config); */
  /* input_handler_destroy(handler); */
  xcb_disconnect(conn);
  printf("===== Menu Demo Exit Success =====\n");
  printf("===== ====================== =====\n");
  return EXIT_SUCCESS;
}
