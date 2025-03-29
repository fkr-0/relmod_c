/* === window_menu.h === */
#ifndef WINDOW_MENU_H
#define WINDOW_MENU_H

#include "menu.h"
#include "x11_window.h"
#include <xcb/xcb.h>

// Forward declaration of WindowList; assumed defined elsewhere.

// Structure that wraps our menu and window list together.
typedef struct {
  xcb_connection_t *conn;  // XCB connection (needed to activate windows)
  Menu *menu;              // Pointer to the created Menu from menu.h API
  WindowList *window_list; // Pointer to our window list
} WindowMenu;

// Creates a window menu using the working menu.h API.
// The created menu is built from the current WindowList.
WindowMenu *window_menu_create(xcb_connection_t *conn, uint16_t modifier_mask,
                               WindowList *window_list);

// Returns the currently selected window id from the menu.
xcb_window_t window_menu_get_selected(WindowMenu *wm);

// Updates the menu items from the latest window list.
// (For example, after window_list_update() is called.)
void window_menu_update_windows(WindowMenu *wm);

// Cleans up all resources allocated by the window menu.
void window_menu_cleanup(WindowMenu *wm);

#endif // WINDOW_MENU_H
