/* menu.c - Core menu implementation */
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Set menu update interval */
void menu_set_update_interval(Menu* menu, unsigned int interval) {
    if (!menu) return;
    menu->update_interval = interval;
}

/* Set menu update callback */
void menu_set_update_callback(Menu* menu, MenuUpdateCallback cb) {
    if (!menu) return;
    menu->update_cb = cb;
}

/* Trigger menu update */
void menu_trigger_update(Menu* menu) {
    if (!menu || !menu->update_cb) return;
    menu->update_cb(menu, menu->user_data);
}

/* Create menu instance */
Menu* menu_create(X11FocusContext* focus_ctx, MenuConfig* config) {
    if (!focus_ctx || !config) return NULL;

    Menu* menu = calloc(1, sizeof(Menu));
    if (!menu) return NULL;

    menu->focus_ctx = focus_ctx;
    menu->conn = focus_ctx->conn;
    menu->config = *config;
    menu->state = MENU_STATE_INACTIVE;
    menu->selected_index = 0;
    menu->update_interval = 0;
    menu->update_cb = NULL;

    return menu;
}

/* Create menu from config */
Menu* menu_create_from_config(MenuConfig* config) {
    if (!config) return NULL;

    Menu* menu = calloc(1, sizeof(Menu));
    if (!menu) return NULL;

    menu->config = *config;
    menu->state = MENU_STATE_INACTIVE;
    menu->selected_index = 0;

    return menu;
}

/* Clean up menu resources */
void menu_destroy(Menu* menu) {
    if (!menu) return;

    // Call cleanup callback if registered
    if (menu->cleanup_cb) {
        menu->cleanup_cb(menu->user_data);
    }

    free(menu);
}

/* Handle key press events */
bool menu_handle_key_press(Menu* menu, xcb_key_press_event_t* event) {
    if (!menu || !event) return false;

    // Update modifier state
    if (event->state & menu->config.mod_key) {
        menu->active_modifier = event->state;
    }

    // Handle navigation keys
    NavigationConfig* nav = &menu->config.nav;

    if (event->detail == nav->next.key) {
        menu_select_next(menu);
        return true;
    }

    if (event->detail == nav->prev.key) {
        menu_select_prev(menu);
        return true;
    }

    // Handle direct selection
    if (menu->config.act.activate_on_direct_key) {
        for (size_t i = 0; i < nav->direct.count; i++) {
            if (event->detail == nav->direct.keys[i]) {
                menu_select_index(menu, i);
                return menu->config.act.activate_on_direct_key;
            }
        }
    }

    // Handle custom action callback
    return menu->action_cb ?
           menu->action_cb(event->detail, menu->user_data) : true;
}

/* Handle key release events */
bool menu_handle_key_release(Menu* menu, xcb_key_release_event_t* event) {
    if (!menu || !event) return false;

    // Handle modifier release activation
    if (menu->config.act.activate_on_mod_release) {
        if ((event->state & menu->active_modifier) == 0) {
            MenuItem* item = menu_get_selected_item(menu);
            if (item && item->action) {
                item->action(item->metadata);
                return false;  // Close menu
            }
        }
    }

    return true;
}

/* Show menu */
void menu_show(Menu* menu) {
    if (!menu || menu->state != MENU_STATE_INACTIVE) return;

    menu->state = MENU_STATE_INITIALIZING;
    menu->active = true;

    // Trigger initial update if dynamic
    if (menu->update_interval > 0 && menu->update_cb) {
        menu_trigger_update(menu);
    }
}

/* Hide menu */
void menu_hide(Menu* menu) {
    if (!menu || menu->state == MENU_STATE_INACTIVE) return;

    menu->state = MENU_STATE_INACTIVE;
    menu->active = false;
    menu->selected_index = 0;
}

/* Select next menu item */
void menu_select_next(Menu* menu) {
    if (!menu || !menu->active || menu->config.item_count == 0) return;
    menu->selected_index = (menu->selected_index + 1) % menu->config.item_count;
}

/* Select previous menu item */
void menu_select_prev(Menu* menu) {
    if (!menu || !menu->active || menu->config.item_count == 0) return;
    menu->selected_index = (menu->selected_index - 1 + menu->config.item_count) %
                          menu->config.item_count;
}

/* Select specific menu item */
void menu_select_index(Menu* menu, int index) {
    if (!menu || !menu->active ||
        index < 0 || index >= (int)menu->config.item_count) return;
    menu->selected_index = index;
}

/* Get currently selected menu item */
MenuItem* menu_get_selected_item(Menu* menu) {
    if (!menu || menu->selected_index < 0 ||
        menu->selected_index >= (int)menu->config.item_count) {
        return NULL;
    }
    return &menu->config.items[menu->selected_index];
}

/* Check if menu is active */
bool menu_is_active(Menu* menu) {
    return menu ? menu->active : false;
}

/* Get current menu state */
MenuState menu_get_state(Menu* menu) {
    return menu ? menu->state : MENU_STATE_INACTIVE;
}
