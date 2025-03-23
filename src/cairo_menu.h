/* cairo_menu.h */

#ifndef CAIRO_MENU_H
#define CAIRO_MENU_H

#include "menu.h"
#include "x11_focus.h"
#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

Menu *cairo_menu_create(xcb_connection_t *conn, xcb_window_t root,
                        X11FocusContext *ctx, xcb_screen_t *screen,
                        const MenuConfig *config);
void cairo_menu_destroy(Menu *menu);
void cairo_menu_show(Menu *menu);
void cairo_menu_hide(Menu *menu);
void cairo_menu_activate(Menu *menu);
void cairo_menu_deactivate(Menu *menu);

// New function to initialize menu configuration
Menu *cairo_menu_init(const MenuConfig *config);

#endif // CAIRO_MENU_H
