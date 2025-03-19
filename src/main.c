/* main.c - Main program entry point */
#include "cairo_menu.h"
#include "input_handler.h"
#include "menu_manager.h"
#include "version.h"
#include "window_menu.h"
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

/* Example action callback */
static void demo_action(void *user_data) {
  const char *item_id = (const char *)user_data;
  printf("Selected item: %s\n", item_id);
}

/* Create an example menu using Cairo */
static Menu *create_demo_menu(xcb_connection_t *conn, xcb_window_t root) {
  static MenuItem items[] = {{.id = "item1",
                              .label = "Option 1",
                              .action = demo_action,
                              .metadata = "item1"},
                             {.id = "item2",
                              .label = "Option 2",
                              .action = demo_action,
                              .metadata = "item2"},
                             {.id = "item3",
                              .label = "Option 3",
                              .action = demo_action,
                              .metadata = "item3"},
                             {.id = "item4",
                              .label = "Option 4",
                              .action = demo_action,
                              .metadata = "item4"}};

  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4, /* Super key */
      .trigger_key = 31,         /* 'i' key */
      .title = "Demo Menu",
      .items = items,
      .item_count = sizeof(items) / sizeof(items[0]),
      .nav = {.next = {.key = 44, .label = "j"},              /* j key */
              .prev = {.key = 45, .label = "k"},              /* k key */
              .direct = {.keys = (uint8_t[]){10, 11, 12, 13}, /* 1-4 keys */
                         .count = 4}},
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = true},
      .style = {.background_color = {0.1, 0.1, 0.1, 0.9},
                .text_color = {0.8, 0.8, 0.8, 1.0},
                .highlight_color = {0.3, 0.3, 0.8, 1.0},
                .font_face = "Sans",
                .font_size = 14.0,
                .item_height = 20,
                .padding = 10}};

  return cairo_menu_create(conn, root, &config);
}

int main(int argc, char *argv[]) {
  printf("X11 Menu System v%s\n", VERSION);

  /* Connect to X server */
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(conn)) {
    fprintf(stderr, "Failed to connect to X server\n");
    return EXIT_FAILURE;
  }

  /* Initialize EWMH */
  xcb_ewmh_connection_t ewmh = {0};
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    fprintf(stderr, "Failed to initialize EWMH\n");
    xcb_disconnect(conn);
    return EXIT_FAILURE;
  }

  /* Get root window */
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t root = screen->root;

  /* Create input handler */
  InputHandler *handler = input_handler_create(conn, &ewmh, root);
  if (!handler) {
    fprintf(stderr, "Failed to create input handler\n");
    xcb_ewmh_connection_wipe(&ewmh);
    xcb_disconnect(conn);
    return EXIT_FAILURE;
  }

  /* Initialize window manager utilities */
  WindowList *window_list = window_list_init(conn);
  if (!window_list) {
    fprintf(stderr, "Failed to initialize window list\n");
    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(&ewmh);
    xcb_disconnect(conn);
    return EXIT_FAILURE;
  }

  /* Create and register menus */
  /* Menu* demo_menu = create_demo_menu(conn, root); */
  /* if (!demo_menu) { */
  /*     fprintf(stderr, "Failed to create demo menu\n"); */
  /*     goto cleanup; */
  /* } */
  /* menu_manager_register(handler->menu_manager, demo_menu); */

  /* Create window menu configuration */
  WindowMenuConfig window_config =
      window_menu_create(conn, root, XCB_MOD_MASK_4, window_list);
  MenuConfig window_menu_config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 31, /* 'w' key */
      .title = "Window Menu",
      .items = NULL, /* No items, handled by window menu */
      .item_count = 0,
      .nav = {.next = {.key = 44, .label = "j"}, /* j key */
              .prev = {.key = 45, .label = "k"}, /* k key */
              .direct = {.keys = NULL, .count = 0}},
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = false},
      .style = {.background_color = {0.1, 0.1, 0.1, 0.9},
                .text_color = {0.8, 0.8, 0.8, 1.0},
                .highlight_color = {0.3, 0.3, 0.8, 1.0},
                .font_face = "Sans",
                .font_size = 14.0,
                .item_height = 20,
                .padding = 10}};
  Menu *window_menu = cairo_menu_create(conn, root, &window_menu_config);

  if (!window_menu) {
    fprintf(stderr, "Failed to create window menu\n");
    goto cleanup;
  }
  menu_manager_register(handler->menu_manager, window_menu);

  /* Main event loop */
  printf("Running menu system. Press 'q' to exit.\n");
  input_handler_run(handler);

cleanup:
  /* Clean up resources */
  window_list_free(window_list);
  input_handler_destroy(
      handler); /* This will clean up menus via menu manager */
  xcb_ewmh_connection_wipe(&ewmh);
  xcb_disconnect(conn);

  return EXIT_SUCCESS;
}
