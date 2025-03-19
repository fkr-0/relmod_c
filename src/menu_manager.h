/* menu_manager.h - Menu management system */
#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include "menu.h"
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <limits.h>
#include <sys/time.h>

/* Forward declarations */
struct MenuManager;
typedef struct MenuManager MenuManager;

/* Menu registry entry */
typedef struct MenuRegistryEntry {
    Menu* menu;
    struct timeval last_update;    /* Last update timestamp */
    struct MenuRegistryEntry* next;
} MenuRegistryEntry;

/* Menu manager context */
struct MenuManager {
    xcb_connection_t* conn;
    xcb_ewmh_connection_t* ewmh;
    X11FocusContext* focus_ctx;
    Menu* active_menu;
    MenuRegistryEntry* registry;
    size_t menu_count;
};

/* Core menu manager API */
MenuManager* menu_manager_create(xcb_connection_t* conn, 
                               xcb_ewmh_connection_t* ewmh);
void menu_manager_destroy(MenuManager* manager);

/* Menu registration */
bool menu_manager_register(MenuManager* manager, Menu* menu);
void menu_manager_unregister(MenuManager* manager, Menu* menu);

/* Input handling */
bool menu_manager_handle_key_press(MenuManager* manager, 
                                 xcb_key_press_event_t* event);
bool menu_manager_handle_key_release(MenuManager* manager, 
                                   xcb_key_release_event_t* event);

/* Menu activation */
bool menu_manager_activate(MenuManager* manager, Menu* menu);
void menu_manager_deactivate(MenuManager* manager);
Menu* menu_manager_get_active(MenuManager* manager);

/* Main event loop */
void menu_manager_run(MenuManager* manager);

/* Helper functions */
size_t menu_manager_get_menu_count(MenuManager* manager);
Menu* menu_manager_find_menu(MenuManager* manager, const char* id);

#endif /* MENU_MANAGER_H */