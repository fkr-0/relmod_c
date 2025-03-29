/* menu.h - Updated to support late injection of X11FocusContext */

#ifndef MENU_H
#define MENU_H

#include "x11_focus.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <xcb/xcb.h>

/* Forward declarations */
typedef struct Menu Menu;
typedef struct MenuConfig MenuConfig;

typedef struct {
  const char *id;
  const char *label;
  void (*action)(void *);
  void *metadata;
  void (*on_select)(void *);
} MenuItem;

typedef struct {
  struct {
    uint8_t key;
    char *label;
  } next;

  struct {
    uint8_t key;
    char *label;
  } prev;

  struct {
    uint8_t *keys;
    size_t count;
  } direct;

  void *extension_data;
} NavigationConfig;

typedef struct {
  bool activate_on_mod_release;
  bool activate_on_direct_key;
  void (*custom_activate)(Menu *, void *);
  void *user_data;
} ActivationConfig;

typedef struct {
  double background_color[4];
  double text_color[4];
  double highlight_color[4];
  char *font_face;
  double font_size;
  int item_height;
  int padding;
} MenuStyle;

/* ActivationState struct */
typedef struct {
  MenuConfig *config;
  uint16_t mod_key;
  uint8_t keycode;
  bool initialized;
  Menu *menu;
} ActivationState;

struct MenuConfig {
  uint16_t mod_key;
  uint8_t trigger_key;
  const char *title;
  MenuItem *items;
  size_t item_count;
  NavigationConfig nav;
  ActivationConfig act;
  MenuStyle style;
  ActivationState act_state;
};

/* Menu state
 * Used to track the current state of the menu for the cases:
 * - Inactive: Menu is not visible
 * - Initializing: Menu is being set up (e.g. rendering)
 * - Active: Menu is visible and active
 * - Navigating: Menu is navigating items (not used)
 * - Activating: Menu is activating an item
 *
 * The state is used to determine how to handle key events and rendering:
 * - Inactive: No key events are handled, no rendering is done
 * - Initializing: No key events are handled, rendering is done
 * - Active: Key events are handled, rendering is done
 * - Navigating: Key events are handled, rendering is done
 * - Activating: No key events are handled, rendering is done
 *
 * Animations and other effects can be added based on the state, typically
 * during rendering. We use the different states to separate the logic for
 * handling key events and rendering.
 * */

typedef enum {
  MENU_STATE_INACTIVE,
  MENU_STATE_INITIALIZING,
  MENU_STATE_ACTIVE,
  MENU_STATE_NAVIGATING,
  MENU_STATE_ACTIVATING
} MenuState;

/* Core menu object */
struct Menu {
  MenuConfig config;
  MenuState state;
  bool active;        // Is the menu active?(the menu receiving key events)
  int selected_index; // Index of the selected item

  void *user_data; // User data pointer - can be used to store custom data
  X11FocusContext *focus_ctx; // X11 focus context - gets passed along the
                              // activation menu for handling focus events

  void (*on_select)(MenuItem *item, void *user_data);
  bool (*activates_cb)(
      uint16_t, uint8_t,
      void *); // Activation callback
               // usage: activates_cb(mod_key, keycode, user_data)
  bool (*action_cb)(uint8_t, void *); // Action callback
                                      // usage: action_cb(keycode, user_data)
  void (*cleanup_cb)(void *);         // Cleanup callback
                                      // usage: cleanup_cb(user_data)
  void (*update_cb)(Menu *, void *);  // Update callback
                                      // usage: update_cb(menu, user_data)
  // triggered onupdate_interval, user events
  // Update interval in milliseconds - used to trigger update callback -0 to
  // disable user events cause update callback to be triggered. update interval
  // operates in the background and gets reset after each update/event
  unsigned int update_interval;
};

/* API */
Menu *menu_create(MenuConfig *config);
void menu_destroy(Menu *menu);

void menu_set_focus_context(Menu *menu, X11FocusContext *ctx);
void menu_show(Menu *menu);
void menu_hide(Menu *menu);
bool menu_handle_key_press(Menu *menu, xcb_key_press_event_t *ev);
bool menu_handle_key_release(Menu *menu, xcb_key_release_event_t *ev);
MenuItem *menu_get_selected_item(Menu *menu);
void menu_select_next(Menu *menu);
void menu_select_prev(Menu *menu);
void menu_select_index(Menu *menu, int index);
bool menu_is_active(Menu *menu);
MenuState menu_get_state(Menu *menu);
void menu_set_update_interval(Menu *menu, unsigned int ms);
void menu_set_update_callback(Menu *menu, void (*cb)(Menu *, void *));
void menu_trigger_update(Menu *menu);
void menu_redraw(Menu *menu);
bool menu_cairo_is_setup(Menu *menu);
void menu_confirm_selection(Menu *menu);
void menu_trigger_on_select(Menu *menu);
void menu_set_on_select_callback(Menu *menu,
                                 void (*on_select)(MenuItem *item,
                                                   void *user_data));
void window_menu_on_select(MenuItem *item, void *user_data);
#endif
