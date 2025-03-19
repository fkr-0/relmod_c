# Menu Extension Plugin Development Guide

This guide explains how to create custom menu plugins for the X11 Menu Extension System.

## Overview

A menu plugin typically consists of:
1. Menu configuration
2. Data management
3. Input handling
4. Resource cleanup

## Basic Plugin Structure

```c
typedef struct {
    // Plugin-specific data
    void* data;
    // Menu items
    MenuItem* items;
    size_t item_count;
    // Update interval (if dynamic)
    unsigned int update_interval;
} PluginData;

// Create menu instance
Menu* create_plugin_menu(xcb_connection_t* conn, xcb_window_t root);
// Cleanup resources
void cleanup_plugin(void* user_data);
// Handle input
bool handle_input(uint8_t keycode, void* user_data);
```

## Example Implementation

Here's a minimal plugin template:

```c
#include "../src/menu.h"
#include "../src/cairo_menu.h"

/* Plugin data structure */
typedef struct {
    char** labels;
    size_t count;
} PluginData;

/* Action callback */
static void plugin_action(void* user_data) {
    PluginData* data = user_data;
    // Handle menu item activation
}

/* Create plugin menu */
Menu* create_plugin_menu(xcb_connection_t* conn, xcb_window_t root) {
    PluginData* data = calloc(1, sizeof(PluginData));
    if (!data) return NULL;
    
    // Initialize plugin data
    data->count = 3;
    data->labels = calloc(data->count, sizeof(char*));
    for (size_t i = 0; i < data->count; i++) {
        data->labels[i] = strdup("Item");
    }
    
    // Create menu items
    MenuItem* items = calloc(data->count, sizeof(MenuItem));
    for (size_t i = 0; i < data->count; i++) {
        items[i] = (MenuItem){
            .id = data->labels[i],
            .label = data->labels[i],
            .action = plugin_action,
            .metadata = data
        };
    }
    
    // Configure menu
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,
        .trigger_key = 44,  // Key to activate menu
        .title = "Plugin Menu",
        .items = items,
        .item_count = data->count,
        .nav = {
            .next = { .key = 44, .label = "j" },
            .prev = { .key = 45, .label = "k" }
        }
    };
    
    return cairo_menu_create(conn, root, &config);
}
```

## Best Practices

### 1. Memory Management

```c
/* Proper cleanup function */
static void cleanup_plugin_data(void* user_data) {
    PluginData* data = user_data;
    if (!data) return;
    
    // Free allocated resources
    for (size_t i = 0; i < data->count; i++) {
        free(data->labels[i]);
    }
    free(data->labels);
    free(data);
}

/* Register cleanup */
menu->cleanup_cb = cleanup_plugin_data;
menu->user_data = data;
```

### 2. Error Handling

```c
Menu* create_plugin_menu(xcb_connection_t* conn, xcb_window_t root) {
    if (!conn) return NULL;
    
    PluginData* data = calloc(1, sizeof(PluginData));
    if (!data) return NULL;
    
    // Initialize with error checking
    if (!initialize_plugin_data(data)) {
        cleanup_plugin_data(data);
        return NULL;
    }
    
    // Create menu with error checking
    Menu* menu = create_menu_with_data(conn, root, data);
    if (!menu) {
        cleanup_plugin_data(data);
        return NULL;
    }
    
    return menu;
}
```

### 3. Dynamic Updates

```c
/* Update callback */
static void update_plugin(Menu* menu, void* user_data) {
    PluginData* data = user_data;
    
    // Update data
    update_plugin_data(data);
    
    // Update menu items
    for (size_t i = 0; i < data->count; i++) {
        menu->config.items[i].label = data->labels[i];
    }
    
    // Request redraw
    menu_redraw(menu);
}

/* Register update timer */
menu->update_interval = 1000;  // milliseconds
menu->update_cb = update_plugin;
```

## Advanced Features

### 1. Nested Menus

```c
typedef struct {
    MenuItem* parent_items;
    MenuItem* child_items;
    size_t parent_count;
    size_t child_count;
} NestedMenuData;

static void create_nested_menu(Menu* parent, int index) {
    // Create child menu
    Menu* child = create_child_menu();
    // Link to parent
    parent->config.items[index].submenu = child;
}
```

### 2. Custom Navigation

```c
static bool custom_navigation(Menu* menu, uint8_t keycode, void* user_data) {
    PluginData* data = user_data;
    
    switch (keycode) {
        case CUSTOM_KEY:
            // Handle custom navigation
            return true;
    }
    
    return false;
}

menu->nav_cb = custom_navigation;
```

### 3. Custom Rendering

```c
static void custom_render(Menu* menu, cairo_t* cr, void* user_data) {
    PluginData* data = user_data;
    
    // Custom drawing code
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

menu->render_cb = custom_render;
```

## Integration

### 1. Register with Menu Manager

```c
int main(void) {
    MenuManager* manager = menu_manager_create(conn, ewmh);
    Menu* plugin = create_plugin_menu(conn, root);
    
    menu_manager_register(manager, plugin);
    menu_manager_run(manager);
}
```

### 2. Plugin Configuration

```c
// Load from config file
static bool load_plugin_config(PluginData* data, const char* config_path) {
    // Read and parse configuration
    return true;
}

// Use in menu creation
Menu* create_plugin_menu(xcb_connection_t* conn, xcb_window_t root) {
    PluginData* data = calloc(1, sizeof(PluginData));
    load_plugin_config(data, "plugin.conf");
    // Create menu with loaded config
}
```

## Testing

### 1. Unit Tests

```c
void test_plugin(void) {
    // Setup
    PluginData* data = create_test_data();
    
    // Test functionality
    assert(data->count == EXPECTED_COUNT);
    
    // Test cleanup
    cleanup_plugin_data(data);
}
```

### 2. Integration Tests

```c
void test_plugin_integration(void) {
    // Create menu
    Menu* menu = create_plugin_menu(conn, root);
    
    // Simulate input
    simulate_key_press(menu, KEY_CODE);
    
    // Verify state
    verify_menu_state(menu);
    
    // Cleanup
    menu_destroy(menu);
}
```

## Common Issues

1. **Memory Leaks**
   - Always pair allocations with frees
   - Use valgrind for testing
   - Clean up in error paths

2. **Input Handling**
   - Check modifier state
   - Handle key repeats
   - Consider focus rules

3. **Resource Management**
   - Cache expensive computations
   - Limit update frequency
   - Handle X11 errors

4. **Thread Safety**
   - Protect shared data
   - Use atomic operations
   - Consider event ordering

## Examples

See the following example plugins:
1. [System Monitor](examples/system_monitor_menu.c)
2. [Window Switcher](examples/window_menu.c)
3. [Application Launcher](examples/launcher_menu.c)

## Support

For questions and issues:
1. Check existing documentation
2. Review example plugins
3. Open GitHub issues
4. Join discussions