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

// Helper: Builds a *new* MenuConfig from the current WindowList.
// Returns a pointer to a newly allocated MenuConfig.
// Caller is responsible for freeing it using menu_config_destroy().
static MenuConfig *build_menu_config(WindowMenu *wm, uint16_t modifier_mask,
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
  // Destroy the builder struct itself *after* finalizing.
  menu_builder_destroy(&builder);
  // Return the pointer to the newly allocated config.
  return config;
}

WindowMenu *window_menu_create(xcb_connection_t *conn, WindowList *window_list,
                               uint16_t modifier_mask, uint8_t trigger_key,
                               xcb_ewmh_connection_t *ewmh, char *title) {
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
  // build_menu_config now returns a pointer to an allocated config
  MenuConfig *config_ptr = build_menu_config(wm, modifier_mask, trigger_key);
  if (!config_ptr) {
    fprintf(stderr, "Failed to build menu config in window_menu_create\n");
    free(wm);           // Free the allocated WindowMenu struct
    exit(EXIT_FAILURE); // Or return NULL if preferred
  }
  // Set the title of the menu.
  if (title) {
    config_ptr->title = strdup(title);
    if (!config_ptr->title) {
      fprintf(stderr, "Failed to allocate memory for menu title\n");
      menu_config_destroy(config_ptr); // Free the config struct
      free(wm);                        // Free the WindowMenu struct
      exit(EXIT_FAILURE);              // Or return NULL if preferred
    }
  } else {
    config_ptr->title = strdup("Window Menu");
  }
  if (!config_ptr->title) {
    fprintf(stderr, "Failed to allocate memory for menu title\n");
    menu_config_destroy(config_ptr); // Free the config struct
    free(wm);                        // Free the WindowMenu struct
    exit(EXIT_FAILURE);              // Or return NULL if preferred
  }

  // Create the menu using the working menu.h API.
  wm->menu = menu_create(config_ptr); // Pass the pointer
  /* menu_set_on_select_callback(wm->menu, window_menu_on_select); */
  if (!wm->menu) {
    fprintf(stderr,
            "Failed to create menu from config in window_menu_create\n");
    menu_config_destroy(config_ptr); // Free the config struct
    free(wm);                        // Free the WindowMenu struct
    exit(EXIT_FAILURE);              // Or return NULL
  }
  // Free the config struct itself after menu_create is done with it
  menu_config_destroy(config_ptr);
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

    // Free old menu item strings (id, label) and metadata before freeing the
    // items array itself. Assumes id and label were previously allocated (e.g.,
    // by strdup or menu_builder).
    if (wm->menu->config
            .items) { // Check if items array exists before freeing contents
      for (size_t i = 0; i < wm->menu->config.item_count; i++) {
        // Check pointers before freeing, belt-and-suspenders approach
        free((void *)wm->menu->config.items[i].id);
        free((void *)wm->menu->config.items[i].label);
        free(wm->menu->config.items[i].metadata);
      }
      free(wm->menu->config.items);  // Free the old items array
      wm->menu->config.items = NULL; // Avoid dangling pointer
    }
    wm->menu->config.item_count = 0; // Reset count before rebuilding

    // Rebuild the menu configuration items from the updated window list.
    wm->menu->config.item_count = wm->window_list->count;

    // Handle case where count is 0
    if (wm->window_list->count == 0) {
      wm->menu->config.items = NULL; // Ensure items is NULL if count is 0
    } else {
      wm->menu->config.items = calloc(wm->window_list->count, sizeof(MenuItem));
      if (!wm->menu->config.items) {
        fprintf(stderr, "Failed to allocate menu items during update\n");
        perror("calloc failed for menu items");
        // Consider less drastic error handling than exit()
        exit(EXIT_FAILURE);
      }

      // Populate the new items array
      bool allocation_failed = false;
      for (size_t i = 0; i < wm->window_list->count; i++) {
        // Duplicate id and label strings
        wm->menu->config.items[i].id =
            strdup(wm->window_list->windows[i].title);
        wm->menu->config.items[i].label =
            strdup(wm->window_list->windows[i].title);

        // Allocate metadata
        xcb_window_t *win_ptr = malloc(sizeof(xcb_window_t));

        // Check allocations
        if (!wm->menu->config.items[i].id || !wm->menu->config.items[i].label ||
            !win_ptr) {
          fprintf(stderr,
                  "Failed to allocate memory for menu item %zu during update\n",
                  i);
          allocation_failed = true;
          // Free any allocations made for this specific item 'i' before
          // breaking
          free((void *)wm->menu->config.items[i].id);
          free((void *)wm->menu->config.items[i].label);
          free(win_ptr); // win_ptr might be NULL here, free(NULL) is safe
          wm->menu->config.items[i].id = NULL;
          wm->menu->config.items[i].label = NULL;
          wm->menu->config.items[i].metadata = NULL;
          break; // Exit the loop on first failure
        }

        *win_ptr = wm->window_list->windows[i].id;
        wm->menu->config.items[i].metadata = win_ptr;
        // Action callback should be set by menu_create or
        // menu_set_on_select_callback We don't need to set it here unless the
        // update logic requires changing it. wm->menu->config.items[i].action =
        // window_menu_on_select; // Example if needed
      }

      // If allocation failed during the loop, cleanup all successfully
      // allocated items in this cycle
      if (allocation_failed) {
        fprintf(stderr,
                "Cleaning up partially allocated items due to failure.\n");
        for (size_t j = 0; j < wm->window_list->count; ++j) {
          // Check if item was successfully allocated before freeing
          if (wm->menu->config.items[j].id || wm->menu->config.items[j].label ||
              wm->menu->config.items[j].metadata) {
            free((void *)wm->menu->config.items[j].id);
            free((void *)wm->menu->config.items[j].label);
            free(wm->menu->config.items[j].metadata);
          } else {
            // Stop cleanup once we hit the item that failed (or subsequent NULL
            // items)
            break;
          }
        }
        free(wm->menu->config.items);
        wm->menu->config.items = NULL;   // Ensure items is NULL after freeing
        wm->menu->config.item_count = 0; // Reset count
        // Consider returning an error code instead of exiting
        exit(EXIT_FAILURE); // Keep exit for now
      }
    }

    // Trigger a redraw of the menu.
    menu_redraw(wm->menu);
  }
}

void window_menu_cleanup(WindowMenu *wm) {
  if (wm) {
    if (wm->menu) {
      // Free metadata for each menu item.
      // Note: If item_count is 0 (empty list), this loop will not execute,
      // which is correct.
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
