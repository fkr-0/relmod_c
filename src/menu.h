/* menu.h - Core menu system interfaces */
#ifndef MENU_H
#define MENU_H

#include "x11_focus.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <xcb/xcb.h>

/* Forward declarations */
typedef struct Menu Menu;
typedef struct MenuManager MenuManager;
typedef struct MenuItem MenuItem;

/* Menu item definition */
struct MenuItem {
    const char* id;                    /* Unique identifier */
    const char* label;                 /* Display text */
    void (*action)(void* user_data);  /* Action callback */
    void* metadata;                    /* Optional metadata */
};

/* Navigation configuration */
typedef struct {
    struct {
        uint8_t key;      /* Key for next item */
        char* label;      /* Key label for display */
    } next;
    
    struct {
        uint8_t key;     /* Key for previous item */
        char* label;     /* Key label for display */
    } prev;
    
    struct {
        uint8_t* keys;   /* Array of direct selection keys */
        size_t count;    /* Number of direct keys */
    } direct;
    
    void* extension_data; /* For future navigation extensions */
} NavigationConfig;

/* Activation configuration */
typedef struct {
    bool activate_on_mod_release;    /* Activate when modifier released */
    bool activate_on_direct_key;     /* Activate on number key press */
    void (*custom_activate)(Menu* menu, void* user_data);  /* Custom activation */
    void* user_data;                /* Data for custom activation */
} ActivationConfig;

/* Menu style configuration */
typedef struct {
    double background_color[4];  /* RGBA background */
    double text_color[4];       /* RGBA text color */
    double highlight_color[4];  /* RGBA selection highlight */
    char* font_face;           /* Font family name */
    double font_size;          /* Font size in points */
    int item_height;          /* Height per item */
    int padding;              /* Internal padding */
} MenuStyle;

/* Menu configuration */
typedef struct {
    uint16_t mod_key;        /* Modifier key (Super, Alt, etc) */
    uint8_t trigger_key;     /* Activation key when mod is pressed */
    const char* title;       /* Menu title */
    MenuItem* items;         /* Array of menu items */
    size_t item_count;       /* Number of items */
    NavigationConfig nav;    /* Navigation configuration */
    ActivationConfig act;    /* Activation behavior */
    MenuStyle style;         /* Visual styling */
} MenuConfig;

/* Menu state tracking */
typedef enum {
    MENU_STATE_INACTIVE,
    MENU_STATE_INITIALIZING,
    MENU_STATE_ACTIVE,
    MENU_STATE_NAVIGATING,
    MENU_STATE_ACTIVATING
} MenuState;

/* Menu lifecycle callbacks */
typedef bool (*MenuActivatesCallback)(uint16_t modifiers, uint8_t keycode, void* user_data);
typedef bool (*MenuActionCallback)(uint8_t keycode, void* user_data);
typedef void (*MenuCleanupCallback)(void* user_data);
typedef void (*MenuUpdateCallback)(Menu* menu, void* user_data);

/* Core menu context */
struct Menu {
    X11FocusContext* focus_ctx;
    xcb_connection_t* conn;
    MenuConfig config;
    MenuState state;
    bool active;
    uint16_t active_modifier;
    int selected_index;
    void* user_data;
    
    /* Callbacks */
    MenuActivatesCallback activates_cb;
    MenuActionCallback action_cb;
    MenuCleanupCallback cleanup_cb;
    MenuUpdateCallback update_cb;
    unsigned int update_interval;  /* milliseconds, 0 for no updates */
};

/* Core menu API */
Menu* menu_create(X11FocusContext* focus_ctx, MenuConfig* config);
void menu_destroy(Menu* menu);

bool menu_handle_key_press(Menu* menu, xcb_key_press_event_t* event);
bool menu_handle_key_release(Menu* menu, xcb_key_release_event_t* event);

void menu_show(Menu* menu);
void menu_hide(Menu* menu);
void menu_select_next(Menu* menu);
void menu_select_prev(Menu* menu);
void menu_select_index(Menu* menu, int index);

/* Helper functions */
MenuItem* menu_get_selected_item(Menu* menu);
bool menu_is_active(Menu* menu);
MenuState menu_get_state(Menu* menu);

/* Update handling */
void menu_set_update_interval(Menu* menu, unsigned int interval);
void menu_set_update_callback(Menu* menu, MenuUpdateCallback cb);
void menu_trigger_update(Menu* menu);

#endif /* MENU_H */
