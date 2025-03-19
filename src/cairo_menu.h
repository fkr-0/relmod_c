/* cairo_menu.h - Cairo-based menu rendering */
#ifndef CAIRO_MENU_H
#define CAIRO_MENU_H

#include "menu.h"
#include "menu_animation.h"
#include <cairo/cairo.h>
#include <xcb/xcb.h>

/* Create a Cairo-rendered menu */
Menu* cairo_menu_create(xcb_connection_t* conn,
                       xcb_window_t parent,
                       MenuConfig* config);

/* Animation configuration */
void cairo_menu_set_animation(Menu* menu, 
                            MenuAnimationType show_anim,
                            MenuAnimationType hide_anim,
                            double duration);

/* Custom animation sequences */
void cairo_menu_set_show_sequence(Menu* menu, 
                                MenuAnimationSequence* sequence);
void cairo_menu_set_hide_sequence(Menu* menu, 
                                MenuAnimationSequence* sequence);

#endif /* CAIRO_MENU_H */
