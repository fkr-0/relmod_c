# Menu Extension Tutorial

This guide walks through creating custom menus using the menu extension system. We'll create a home directory browser menu as a practical example.

## API Reference

### Required Components

#### 1. Menu Configuration

```c
typedef struct {
    // Required fields
    uint16_t mod_key;      // X11 modifier mask (e.g., XCB_MOD_MASK_4 for Super)
    uint8_t trigger_key;   // X11 keycode for activation
    const char* title;     // Menu title string
    MenuItem* items;       // Array of menu items
    size_t item_count;     // Number of items in array
    
    // Optional fields
    NavigationConfig nav;  // Custom navigation controls
    ActivationConfig act;  // Activation behavior
    MenuStyle style;       // Visual styling
} MenuConfig;

// Menu item definition
typedef struct {
    const char* id;        // Unique identifier string
    const char* label;     // Display text
    void (*action)(void* user_data);  // Callback when item activated
    void* metadata;        // Optional data passed to action
} MenuItem;
```

#### 2. Required Callbacks

```c
// Menu activation check (required)
bool (*MenuActivatesCallback)(
    uint16_t modifiers,    // Current modifier state
    uint8_t keycode,       // Pressed key
    void* user_data       // Menu's user data
);

// Menu action handler (required)
bool (*MenuActionCallback)(
    uint8_t keycode,      // Pressed/released key
    void* user_data      // Menu's user data
);

// Cleanup handler (required)
void (*MenuCleanupCallback)(
    void* user_data      // Menu's user data
);
```

### Optional Components

#### 1. Navigation Configuration

```c
typedef struct {
    struct {
        uint8_t key;      // Key for next item (default: 'j')
        char* label;      // Key label for display
    } next;
    
    struct {
        uint8_t key;      // Key for previous item (default: 'k')
        char* label;      // Key label for display
    } prev;
    
    struct {
        uint8_t* keys;    // Array of direct selection keys (default: 1-4)
        size_t count;     // Number of direct keys
    } direct;
} NavigationConfig;
```

#### 2. Activation Configuration

```c
typedef struct {
    bool activate_on_mod_release;    // Activate on modifier release
    bool activate_on_direct_key;     // Activate on number key press
    void (*custom_activate)(Menu* menu, void* user_data);  // Custom activation
    void* user_data;                // Data for custom activation
} ActivationConfig;
```

#### 3. Style Configuration

```c
typedef struct {
    double background_color[4];  // RGBA background
    double text_color[4];       // RGBA text color
    double highlight_color[4];  // RGBA selection highlight
    char* font_face;           // Font family name
    double font_size;          // Font size in points
    int item_height;          // Height per item
    int padding;              // Internal padding
} MenuStyle;
```

## Tutorial: Home Directory Browser Menu

Let's create a menu that:
1. Lists contents of the home directory
2. Allows navigation with j/k keys
3. Opens selected files/directories with xdg-open

### Step 1: Create Menu Data Structure

```c
// Menu state/data
typedef struct {
    char** items;           // Array of file/directory names
    char** full_paths;      // Array of full paths
    size_t count;           // Number of items
    int selected;           // Currently selected index
    uint16_t mod_mask;      // Stored modifier mask
} HomeDirData;

// Helper to free resources
static void home_dir_cleanup(void* user_data) {
    HomeDirData* data = user_data;
    for (size_t i = 0; i < data->count; i++) {
        free(data->items[i]);
        free(data->full_paths[i]);
    }
    free(data->items);
    free(data->full_paths);
    free(data);
}
```

### Step 2: Implement Menu Callbacks

```c
// Activation check
static bool home_dir_activates(uint16_t modifiers, uint8_t keycode, void* user_data) {
    HomeDirData* data = user_data;
    // Activate on Super+H
    return (modifiers & XCB_MOD_MASK_4) && keycode == XK_H;
}

// Input handling
static bool home_dir_action(uint8_t keycode, void* user_data) {
    HomeDirData* data = user_data;
    
    switch (keycode) {
        case XK_j:  // Next item
            data->selected = (data->selected + 1) % data->count;
            return true;
            
        case XK_k:  // Previous item
            data->selected = (data->selected - 1 + data->count) % data->count;
            return true;
            
        case XK_Return:  // Open selected item
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "xdg-open '%s'", 
                    data->full_paths[data->selected]);
            system(cmd);
            return false;  // Close menu
            
        case XK_Escape:
            return false;  // Close menu
    }
    return true;
}
```

### Step 3: Create Menu Configuration

```c
// Helper to scan home directory
static HomeDirData* scan_home_directory(void) {
    HomeDirData* data = calloc(1, sizeof(HomeDirData));
    const char* home = getenv("HOME");
    
    // Open directory
    DIR* dir = opendir(home);
    if (!dir) {
        free(data);
        return NULL;
    }
    
    // Count entries
    struct dirent* entry;
    size_t capacity = 32;
    data->items = malloc(capacity * sizeof(char*));
    data->full_paths = malloc(capacity * sizeof(char*));
    
    // Read entries
    while ((entry = readdir(dir))) {
        // Skip hidden files and . / ..
        if (entry->d_name[0] == '.') continue;
        
        // Resize if needed
        if (data->count == capacity) {
            capacity *= 2;
            data->items = realloc(data->items, capacity * sizeof(char*));
            data->full_paths = realloc(data->full_paths, capacity * sizeof(char*));
        }
        
        // Store name and full path
        data->items[data->count] = strdup(entry->d_name);
        asprintf(&data->full_paths[data->count], "%s/%s", home, entry->d_name);
        data->count++;
    }
    
    closedir(dir);
    return data;
}

// Create menu configuration
MenuConfig create_home_dir_menu(void) {
    HomeDirData* data = scan_home_directory();
    if (!data) {
        fprintf(stderr, "Failed to scan home directory\n");
        exit(1);
    }
    
    // Create menu items
    MenuItem* items = malloc(data->count * sizeof(MenuItem));
    for (size_t i = 0; i < data->count; i++) {
        items[i] = (MenuItem){
            .id = data->full_paths[i],
            .label = data->items[i],
            .action = NULL,  // Handled in action callback
            .metadata = NULL
        };
    }
    
    // Configure navigation
    NavigationConfig nav = {
        .next = { .key = XK_j, .label = "j" },
        .prev = { .key = XK_k, .label = "k" }
    };
    
    // Configure activation
    ActivationConfig act = {
        .activate_on_mod_release = false,
        .activate_on_direct_key = false
    };
    
    return (MenuConfig){
        .mod_key = XCB_MOD_MASK_4,  // Super key
        .trigger_key = XK_H,         // 'H' key
        .title = "Home Directory",
        .items = items,
        .item_count = data->count,
        .nav = nav,
        .act = act
    };
}
```

### Step 4: Register with Menu Manager

```c
int main(int argc, char* argv[]) {
    // Create X11 connection
    xcb_connection_t* conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to X server\n");
        return 1;
    }
    
    // Initialize menu manager
    MenuManager* manager = menu_manager_create(conn);
    
    // Create and register home directory menu
    MenuConfig home_menu = create_home_dir_menu();
    menu_manager_register(manager, &home_menu);
    
    // Run event loop
    menu_manager_run(manager);
    
    // Cleanup
    menu_manager_destroy(manager);
    xcb_disconnect(conn);
    return 0;
}
```

## Usage

Once implemented, the home directory browser menu can be used:

1. Press Super+H to open the menu
2. Use j/k to navigate items
3. Press Enter to open selected item
4. Press Escape to dismiss

## Adding Animations to Your Menu

The menu system supports configurable animations for showing and hiding menus:

```c
// Set basic fade animation (200ms duration)
cairo_menu_set_animation(menu, 
    MENU_ANIM_FADE,    // Show animation
    MENU_ANIM_FADE,    // Hide animation
    200.0);            // Duration in ms

// Create a custom animation sequence
MenuAnimationSequence* seq = menu_animation_sequence_create();
menu_animation_sequence_add(seq, menu_animation_fade_in(100));
menu_animation_sequence_add(seq, menu_animation_slide_in(MENU_ANIM_SLIDE_RIGHT, 100));
cairo_menu_set_show_sequence(menu, seq);
```

## Best Practices

1. **Resource Management**
   - Always implement cleanup callbacks
   - Free resources in reverse allocation order
   - Check for allocation failures

2. **Error Handling**
   - Validate all inputs
   - Handle syscall failures
   - Clean up on initialization failures

3. **User Experience**
   - Provide clear visual feedback
   - Keep menu response time < 16ms
   - Support keyboard navigation

4. **Code Organization**
   - Group related functionality
   - Use clear naming conventions
   - Document public interfaces

## Common Pitfalls

1. **Memory Management**
   - Double frees
   - Memory leaks
   - Buffer overflows

2. **X11 Interactions**
   - Missing error checks
   - Incorrect event handling
   - Resource leaks

3. **Thread Safety**
   - Access to shared data
   - Event handling races
   - Resource cleanup races

## Advanced Topics

1. **Custom Navigation**
   - Implement search functionality
   - Add pagination support
   - Create custom key bindings

2. **Dynamic Updates**
   - Monitor directory changes
   - Update menu items live
   - Handle item removal

3. **Visual Customization**
   - Custom item rendering
   - Animations
   - Theme support
