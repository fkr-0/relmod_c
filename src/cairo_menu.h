#ifndef CAIRO_MENU_H
#define CAIRO_MENU_H

#include "menu.h"
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>

MenuConfig cairo_menu_create(xcb_connection_t *conn, xcb_window_t parent_window,
                             uint16_t modifier_mask, char **items,
                             int item_count);

#endif
