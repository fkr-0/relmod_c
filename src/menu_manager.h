/* menu_manager.h - Menu management system */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include "cairo_menu.h"
#include "menu.h"
#include "x11_focus.h"
#include <stdbool.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

/* Forward declaration */
typedef struct MenuManager MenuManager;

/* Menu manager context */
struct MenuManager {
  xcb_connection_t *conn;
  xcb_ewmh_connection_t *ewmh;
  X11FocusContext *focus_ctx;
  Menu *active_menu;
  void *registry; // now opaque
  size_t menu_count;
};

/* Core API */
MenuManager *menu_manager_create();
MenuManager *menu_manager_connect(MenuManager *mgr, xcb_connection_t *conn,
                                  X11FocusContext *focus_ctx,
                                  xcb_ewmh_connection_t *ewmh);

bool menu_manager_is_connected(MenuManager *mgr);
void menu_manager_destroy(MenuManager *manager);

bool menu_manager_register(MenuManager *manager, Menu *menu);
Menu *menu_manager_menu_index(MenuManager *manager, size_t index);
void menu_manager_unregister(MenuManager *manager, Menu *menu);

bool menu_manager_activate(MenuManager *manager, Menu *menu);
void menu_manager_deactivate(MenuManager *manager);
Menu *menu_manager_get_active(MenuManager *manager);

size_t menu_manager_get_menu_count(MenuManager *manager);
Menu *menu_manager_find_menu(MenuManager *manager, const char *id);
char *menu_manager_status_string(MenuManager *manager); // caller must free

/* Registry iteration (internalized access) */
typedef bool (*MenuManagerForEachFn)(Menu *menu, struct timeval *last_update,
                                     void *user_data);
/* Iterate over all menus in the manager
 * Usage: menu_manager_foreach(manager, fn, user_data)
 * fn: MenuManagerForEachFn - callback function accepting Menu, timeval, void*
 *     - return false to stop iteration
 * user_data user_data: void* - user data to pass to callback
 *
 * Returns: void
 *
 * Example: find matching mod_key + keycode:
 * pass ev from key press event and match mod_key + keycode
 * bool fn(Menu *menu, struct timeval *last_update, void *user_data) {
    *   xcb_key_press_event_t *ev = (xcb_key_press_event_t *)user_data;

    *   if (menu->mod_key == ev->state && menu->trigger_key == ev->detail) {
    *     menu_manager_activate(manager, menu) {
    *     return false; // Stop iteration
    *   }
    * return true; // Continue iteration
    * }
    *
    * xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
    * menu_manager_foreach(manager, fn, ev);
 * */
void menu_manager_foreach(MenuManager *manager, MenuManagerForEachFn fn,
                          void *user_data);

#endif /* MENU_MANAGER_H */
