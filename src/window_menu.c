/* === window_menu.c === */
#include "window_menu.h"
#include "cairo_menu_render.h"
#include "menu.h"
#include "menu_builder.h"
#include "window_menu.h" // self-include for consistency
#include "x11_window.h"  // assuming window_activate is declared here
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[WINMEN]"
#endif
#include "log.h"

// Helper: on-select callback for the menu.
// When an item is selected, this callback is invoked to activate the
// corresponding window.
void window_menu_on_select(MenuItem *item, void *user_data) {
  WindowMenu *wm = (WindowMenu *)user_data;
  if (wm && item && item->metadata) {
    // metadata is stored as a pointer to xcb_window_t
    xcb_window_t win = *((xcb_window_t *)item->metadata);
    // Activate the window (function assumed to be provided elsewhere)
    /* focus_window(wm->conn, *wm->menu->focus_ctx->ewmh, win); */
    uint32_t desktop = window_get_desktop(wm->conn, win);
    LOG("OnSelect Window: %u, desktop: %u", win, desktop);
    window_activate(wm->conn, win);
    switch_to_window(wm->conn, win);
    /* CairoMenuData *data = (CairoMenuData *)wm->menu->user_data; */
    /* cairo_menu_render_request_update(data); */
    /* menu_hide(wm->menu); */
    /* menu_show(wm->menu); */
  }
}

// Helper: (Re)builds the MenuConfig items from the current WindowList.
static MenuConfig build_menu_config(WindowMenu *wm, uint16_t modifier_mask,
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
  menu_builder_set_mod_key(&builder, modifier_mask);
  menu_builder_set_navigation_keys(&builder, 44, "j", 45, "k", NULL, 0); // j, k
  menu_builder_set_activation(&builder, true, true);
  MenuConfig *config = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return *config;
}

WindowMenu *window_menu_create(xcb_connection_t *conn, WindowList *window_list,
                               uint16_t modifier_mask, uint8_t trigger_key,
                               xcb_ewmh_connection_t *ewmh) {
  // Allocate the WindowMenu structure.
  WindowMenu *wm = calloc(1, sizeof(WindowMenu));
  if (!wm) {
    fprintf(stderr, "Failed to allocate WindowMenu\n");
    exit(EXIT_FAILURE);
  }
  wm->conn = conn;
  wm->window_list = window_list;
  wm->ewmh = ewmh; // Store the ewmh pointer
  // Build the MenuConfig from the window list.
  MenuConfig config = build_menu_config(wm, modifier_mask, trigger_key);

  // Create the menu using the working menu.h API.
  wm->menu = menu_create(&config);
  /* menu_set_on_select_callback(wm->menu, window_menu_on_select); */
  if (!wm->menu) {
    fprintf(stderr, "Failed to create menu\n");
    free(wm);
    exit(EXIT_FAILURE);
  }
  // Set the on-select callback so that any selection activates the window.
  menu_set_on_select_callback(wm->menu, window_menu_on_select);
  wm->menu->on_select = window_menu_on_select;
  // Ensure the WindowMenu pointer is available in the menu’s user_data.
  wm->menu->user_data = wm;

  return wm;
}

xcb_window_t window_menu_get_selected(WindowMenu *wm) {
  LOG("WindowMenu Get Selected");
  if (wm && wm->menu) {
    MenuItem *item = menu_get_selected_item(wm->menu);
    if (item && item->metadata)
      return *((xcb_window_t *)item->metadata);
  }
  return XCB_NONE;
}

void window_menu_update_windows(WindowMenu *wm) {
  if (wm && wm->menu && wm->window_list) {
    // Update the window list (function provided elsewhere).
    window_list_update(wm->window_list, wm->conn, wm->ewmh); // Pass stored ewmh
    // Free existing menu item metadata.
    for (size_t i = 0; i < wm->menu->config.item_count; i++) {
      free(wm->menu->config.items[i].metadata);
    }
    free(wm->menu->config.items);
    // Rebuild the menu configuration items from the updated window list.
    wm->menu->config.item_count = wm->window_list->count;
    wm->menu->config.items = calloc(wm->window_list->count, sizeof(MenuItem));
    if (!wm->menu->config.items) {
      fprintf(stderr, "Failed to allocate menu items during update\n");
      exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < wm->window_list->count; i++) {
      wm->menu->config.items[i].id = wm->window_list->windows[i].title;
      wm->menu->config.items[i].label = wm->window_list->windows[i].title;
      xcb_window_t *win_ptr = malloc(sizeof(xcb_window_t));
      if (!win_ptr) {
        fprintf(stderr,
                "Failed to allocate memory for window id during update\n");
        exit(EXIT_FAILURE);
      }
      *win_ptr = wm->window_list->windows[i].id;
      wm->menu->config.items[i].metadata = win_ptr;
    }
    // Trigger a redraw of the menu.
    menu_redraw(wm->menu);
  }
}

void window_menu_cleanup(WindowMenu *wm) {
  if (wm) {
    if (wm->menu) {
      // Free metadata for each menu item.
      for (size_t i = 0; i < wm->menu->config.item_count; i++) {
        free(wm->menu->config.items[i].metadata);
      }
      free(wm->menu->config.items);
      // Destroy the menu via the menu API.
      menu_destroy(wm->menu);
    }
    free(wm);
  }
}
