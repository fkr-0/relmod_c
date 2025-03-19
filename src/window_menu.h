#ifndef WINDOW_MENU_H
#define WINDOW_MENU_H

#include "menu.h"
#include "x11_window.h"
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>

// Window menu configuration structure
typedef struct {
    uint16_t modifier_mask;
    bool (*activate_cb)(uint16_t modifiers, uint8_t keycode, void* user_data);
    bool (*action_cb)(uint8_t keycode, void* user_data);
    void (*cleanup_cb)(void* user_data);
    void* user_data;
} WindowMenuConfig;

// Create a window management menu that displays and controls window list
WindowMenuConfig window_menu_create(xcb_connection_t *conn,
                              xcb_window_t parent_window,
                              uint16_t modifier_mask, WindowList *window_list);

// Get the currently selected window from the menu
xcb_window_t window_menu_get_selected(void *user_data);

// Update the window list in the menu
void window_menu_update_windows(void *user_data);

#endif // WINDOW_MENU_H
