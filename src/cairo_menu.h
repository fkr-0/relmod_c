/* cairo_menu.h */

#ifndef CAIRO_MENU_H
#define CAIRO_MENU_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include "menu.h"

Menu *cairo_menu_create(xcb_connection_t *conn, xcb_window_t root,
                        const MenuConfig *config);
void cairo_menu_destroy(Menu *menu);
void cairo_menu_show(Menu *menu);
void cairo_menu_hide(Menu *menu);

// New function to initialize menu configuration
Menu *cairo_menu_init(const MenuConfig *config);

#endif // CAIRO_MENU_H
