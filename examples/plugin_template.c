/* plugin_template.c - Template for creating menu plugins */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>

/* Plugin-specific data structure */
typedef struct {
    char** labels;             // Dynamic menu labels
    void** metadata;           // Item-specific data
    size_t count;             // Number of items
    unsigned int interval;     // Update interval (ms), 0 for static
    void* plugin_state;       // Plugin-specific state
} PluginData;

/* Forward declarations of plugin callbacks */
static void plugin_cleanup(void* user_data);
static void plugin_update(void* user_data);
static void plugin_action(void* user_data);
static bool plugin_handle_input(uint8_t keycode, void* user_data);

/* Initialize plugin data */
static PluginData* plugin_data_create(void) {
    PluginData* data = calloc(1, sizeof(PluginData));
    if (!data) return NULL;
    
    // TODO: Initialize your plugin data here
    data->count = 1;  // Number of menu items
    data->interval = 0;  // Update interval (0 for static menu)
    
    // Allocate arrays
    data->labels = calloc(data->count, sizeof(char*));
    data->metadata = calloc(data->count, sizeof(void*));
    
    if (!data->labels || !data->metadata) {
        plugin_cleanup(data);
        return NULL;
    }
    
    // Initialize items
    data->labels[0] = strdup("Template Item");
    // data->metadata[0] = your_metadata;
    
    return data;
}

/* Clean up plugin resources */
static void plugin_cleanup(void* user_data) {
    PluginData* data = user_data;
    if (!data) return;
    
    // Free labels
    if (data->labels) {
        for (size_t i = 0; i < data->count; i++) {
            free(data->labels[i]);
        }
        free(data->labels);
    }
    
    // Free metadata
    if (data->metadata) {
        for (size_t i = 0; i < data->count; i++) {
            free(data->metadata[i]);
        }
        free(data->metadata);
    }
    
    // Free plugin state
    free(data->plugin_state);
    
    free(data);
}

/* Update menu contents */
static void plugin_update(void* user_data) {
    PluginData* data = user_data;
    if (!data) return;
    
    // TODO: Update your menu items here
    // Example:
    // snprintf(data->labels[0], MAX_LABEL_LEN, "Updated: %d", value);
}

/* Handle menu item activation */
static void plugin_action(void* user_data) {
    // TODO: Handle menu item activation
    printf("Menu item activated\n");
}

/* Handle custom input */
static bool plugin_handle_input(uint8_t keycode, void* user_data) {
    PluginData* data = user_data;
    if (!data) return false;
    
    // TODO: Handle custom key input
    // Return true if handled, false to pass to default handler
    return false;
}

/* Create menu items */
static MenuItem* create_menu_items(PluginData* data) {
    MenuItem* items = calloc(data->count, sizeof(MenuItem));
    if (!items) return NULL;
    
    for (size_t i = 0; i < data->count; i++) {
        items[i] = (MenuItem){
            .id = data->labels[i],
            .label = data->labels[i],
            .action = plugin_action,
            .metadata = data->metadata[i]
        };
    }
    
    return items;
}

/* Create plugin menu */
Menu* create_plugin_menu(xcb_connection_t* conn, xcb_window_t root) {
    // Create and initialize plugin data
    PluginData* data = plugin_data_create();
    if (!data) return NULL;
    
    // Create menu items
    MenuItem* items = create_menu_items(data);
    if (!items) {
        plugin_cleanup(data);
        return NULL;
    }
    
    // Configure menu
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,    /* Super key */
        .trigger_key = 44,             /* Change this to your preferred key */
        .title = "Plugin Template",
        .items = items,
        .item_count = data->count,
        .nav = {
            .next = { .key = 44, .label = "j" },
            .prev = { .key = 45, .label = "k" },
            .direct = {
                .keys = (uint8_t[]){10, 11, 12, 13},  /* 1-4 keys */
                .count = 4
            }
        },
        .act = {
            .activate_on_mod_release = true,
            .activate_on_direct_key = true
        }
    };
    
    // Create menu
    menu_setup_cairo(conn, root, &config);
    if (!menu) {
        free(items);
        plugin_cleanup(data);
        return NULL;
    }
    
    // Set up menu callbacks
    menu->cleanup_cb = plugin_cleanup;
    menu->action_cb = plugin_handle_input;
    menu->user_data = data;
    
    // Set up updates if needed
    if (data->interval > 0) {
        // TODO: Set up timer for plugin_update
    }
    
    free(items);  // Items are copied by menu_create
    return menu;
}

/* Example usage */
#ifdef PLUGIN_TEST
int main(void) {
    xcb_connection_t* conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to X server\n");
        return 1;
    }
    
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    Menu* menu = create_plugin_menu(conn, screen->root);
    
    if (!menu) {
        fprintf(stderr, "Failed to create plugin menu\n");
        xcb_disconnect(conn);
        return 1;
    }
    
    printf("Plugin menu created successfully\n");
    printf("Press Super+Key to activate\n");
    
    // Menu would be registered with menu manager here
    // menu_manager_register(manager, menu);
    
    // Cleanup for test
    menu_destroy(menu);
    xcb_disconnect(conn);
    
    return 0;
}
#endif