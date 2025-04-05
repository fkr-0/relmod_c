/* plugin_template.c - Template for creating menu plugins */
#include "../src/menu.h"
#include "../src/cairo_menu.h" // Include for potential rendering setup elsewhere
#include "../src/input_handler.h" // Include for main example
#include <stdbool.h> // For bool type
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

    // Create menu using the config
    Menu* menu = menu_create(&config);

    // Check if menu creation failed
    if (!menu) {
        // Items array is already freed by menu_create on failure internally,
        // but we need to free our allocated plugin data.
        plugin_cleanup(data);
        // Also free the items array we passed to config, as menu_create failed
        for (size_t i = 0; i < data->count; i++) {
             free((void*)items[i].id); // Assuming id was allocated
             free((void*)items[i].label); // Assuming label was allocated
        }
        free(items);
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

    // Free the temporary items array we created (menu_create copies it)
    // Assuming id and label were allocated and need freeing
     for (size_t i = 0; i < data->count; i++) {
         // free((void*)items[i].id); // Free if allocated
         // free((void*)items[i].label); // Free if allocated
     }
    free(items);

    return menu;
}

/* Example usage */
#ifdef PLUGIN_TEST
int main(void) {
    InputHandler *handler = input_handler_create();
     if (!handler) {
         fprintf(stderr, "Failed to create input handler\n");
         return 1;
     }
     if (!input_handler_setup_x(handler)) {
          fprintf(stderr, "Failed to setup X for input handler\n");
          input_handler_destroy(handler);
          return 1;
     }

    // Create the plugin menu instance
    Menu* menu = create_plugin_menu(); // Assuming create_plugin_menu is updated
                                       // to not require conn/root directly

    if (!menu) {
        fprintf(stderr, "Failed to create plugin menu\n");
        input_handler_destroy(handler);
        return 1;
    }

    // Register the menu with the input handler
    input_handler_add_menu(handler, menu);

    printf("Plugin menu created and registered successfully\n");
    printf("Press Super+J (or configured key) to activate\n"); // Adjust key hint
    printf("Press ESC or q to exit.\n");

    // Run the event loop
    input_handler_run(handler);

    // Cleanup
    printf("Exiting...\n");
    input_handler_destroy(handler); // Destroys handler and registered menus

    return 0;
}
#endif
