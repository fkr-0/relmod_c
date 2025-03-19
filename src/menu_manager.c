/* menu_manager.c - Menu management implementation */
#include "menu_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>

#ifndef UINT_MAX
#define UINT_MAX (~(unsigned int)0)
#endif

/* Private helper functions */
static void registry_cleanup(MenuRegistryEntry* entry) {
    while (entry) {
        MenuRegistryEntry* next = entry->next;
        menu_destroy(entry->menu);
        free(entry);
        entry = next;
    }
}

static MenuRegistryEntry* registry_find(MenuManager* manager, Menu* menu) {
    MenuRegistryEntry* entry = manager->registry;
    while (entry) {
        if (entry->menu == menu) return entry;
        entry = entry->next;
    }
    return NULL;
}

/* Find the shortest update interval among active menus */
static unsigned int find_next_update_interval(MenuManager* manager) {
    unsigned int min_interval = UINT_MAX;
    MenuRegistryEntry* entry = manager->registry;
    
    while (entry) {
        Menu* menu = entry->menu;
        if (menu->active && menu->update_interval > 0) {
            if (menu->update_interval < min_interval) {
                min_interval = menu->update_interval;
            }
        }
        entry = entry->next;
    }
    
    return min_interval == UINT_MAX ? 0 : min_interval;
}

/* Update active menus */
static void update_active_menus(MenuManager* manager) {
    MenuRegistryEntry* entry = manager->registry;
    struct timeval now;
    gettimeofday(&now, NULL);
    
    while (entry) {
        Menu* menu = entry->menu;
        if (menu->active && menu->update_interval > 0 && menu->update_cb) {
            // Check if it's time to update
            unsigned long elapsed = 
                (now.tv_sec - entry->last_update.tv_sec) * 1000 +
                (now.tv_usec - entry->last_update.tv_usec) / 1000;
                
            if (elapsed >= menu->update_interval) {
                menu_trigger_update(menu);
                entry->last_update = now;
            }
        }
        entry = entry->next;
    }
}

/* Core menu manager API implementation */
MenuManager* menu_manager_create(xcb_connection_t* conn, xcb_ewmh_connection_t* ewmh) {
    if (!conn || !ewmh) {
        fprintf(stderr, "Invalid connection parameters\n");
        return NULL;
    }

    MenuManager* manager = calloc(1, sizeof(MenuManager));
    if (!manager) {
        fprintf(stderr, "Failed to allocate menu manager\n");
        return NULL;
    }

    manager->conn = conn;
    manager->ewmh = ewmh;
    manager->focus_ctx = x11_focus_init(conn, ewmh);
    
    if (!manager->focus_ctx) {
        free(manager);
        return NULL;
    }

    return manager;
}

void menu_manager_destroy(MenuManager* manager) {
    if (!manager) return;

    if (manager->active_menu) {
        menu_hide(manager->active_menu);
    }

    registry_cleanup(manager->registry);
    x11_focus_cleanup(manager->focus_ctx);
    free(manager);
}

bool menu_manager_register(MenuManager* manager, Menu* menu) {
    if (!manager || !menu) return false;

    // Check for duplicate registration
    if (registry_find(manager, menu)) {
        fprintf(stderr, "Menu already registered\n");
        return false;
    }

    MenuRegistryEntry* entry = malloc(sizeof(MenuRegistryEntry));
    if (!entry) {
        fprintf(stderr, "Failed to allocate registry entry\n");
        return false;
    }

    entry->menu = menu;
    entry->next = manager->registry;
    gettimeofday(&entry->last_update, NULL);
    manager->registry = entry;
    manager->menu_count++;

    return true;
}

void menu_manager_unregister(MenuManager* manager, Menu* menu) {
    if (!manager || !menu) return;

    if (manager->active_menu == menu) {
        menu_manager_deactivate(manager);
    }

    MenuRegistryEntry** curr = &manager->registry;
    while (*curr) {
        MenuRegistryEntry* entry = *curr;
        if (entry->menu == menu) {
            *curr = entry->next;
            free(entry);
            manager->menu_count--;
            return;
        }
        curr = &entry->next;
    }
}

bool menu_manager_handle_key_press(MenuManager* manager, xcb_key_press_event_t* event) {
    if (!manager || !event) return false;

    // Handle active menu
    if (manager->active_menu) {
        return menu_handle_key_press(manager->active_menu, event);
    }

    // Check for menu activation
    MenuRegistryEntry* entry = manager->registry;
    while (entry) {
        if (entry->menu->activates_cb &&
            entry->menu->activates_cb(event->state, event->detail, 
                                    entry->menu->user_data)) {
            return menu_manager_activate(manager, entry->menu);
        }
        entry = entry->next;
    }

    return false;
}

bool menu_manager_handle_key_release(MenuManager* manager, xcb_key_release_event_t* event) {
    if (!manager || !event) return false;

    if (manager->active_menu) {
        if (!menu_handle_key_release(manager->active_menu, event)) {
            menu_manager_deactivate(manager);
            return true;
        }
    }

    return false;
}

bool menu_manager_activate(MenuManager* manager, Menu* menu) {
    if (!manager || !menu) return false;

    // Ensure menu is registered
    MenuRegistryEntry* entry = registry_find(manager, menu);
    if (!entry) {
        fprintf(stderr, "Cannot activate unregistered menu\n");
        return false;
    }

    // Deactivate current menu if any
    if (manager->active_menu) {
        menu_manager_deactivate(manager);
    }

    // Update timestamp before activation
    gettimeofday(&entry->last_update, NULL);
    
    menu_show(menu);
    manager->active_menu = menu;
    return true;
}

void menu_manager_deactivate(MenuManager* manager) {
    if (!manager || !manager->active_menu) return;

    menu_hide(manager->active_menu);
    manager->active_menu = NULL;
}

Menu* menu_manager_get_active(MenuManager* manager) {
    return manager ? manager->active_menu : NULL;
}

void menu_manager_run(MenuManager* manager) {
    if (!manager) return;

    int xcb_fd = xcb_get_file_descriptor(manager->conn);
    bool running = true;

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(xcb_fd, &fds);

        // Calculate timeout for next update
        struct timeval timeout = {0};
        unsigned int update_interval = find_next_update_interval(manager);
        
        if (update_interval > 0) {
            timeout.tv_sec = update_interval / 1000;
            timeout.tv_usec = (update_interval % 1000) * 1000;
        }

        int ret = select(xcb_fd + 1, &fds, NULL, NULL, 
                        update_interval > 0 ? &timeout : NULL);

        if (ret > 0) {
            // Handle X11 events
            xcb_generic_event_t* event;
            while ((event = xcb_poll_for_event(manager->conn))) {
                switch (event->response_type & ~0x80) {
                    case XCB_KEY_PRESS:
                        menu_manager_handle_key_press(manager, 
                            (xcb_key_press_event_t*)event);
                        break;

                    case XCB_KEY_RELEASE:
                        menu_manager_handle_key_release(manager, 
                            (xcb_key_release_event_t*)event);
                        break;
                }
                free(event);
            }
        } else if (ret == 0 && update_interval > 0) {
            // Handle menu updates
            update_active_menus(manager);
        }

        // Check for X11 connection errors
        if (xcb_connection_has_error(manager->conn)) {
            fprintf(stderr, "X11 connection error\n");
            break;
        }
    }
}

size_t menu_manager_get_menu_count(MenuManager* manager) {
    return manager ? manager->menu_count : 0;
}

Menu* menu_manager_find_menu(MenuManager* manager, const char* id) {
    if (!manager || !id) return NULL;

    MenuRegistryEntry* entry = manager->registry;
    while (entry) {
        if (strcmp(entry->menu->config.title, id) == 0) {
            return entry->menu;
        }
        entry = entry->next;
    }
    return NULL;
}