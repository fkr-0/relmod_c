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
  bool active;
  int selected_index;

  void *user_data;
  X11FocusContext *focus_ctx;

  bool (*activates_cb)(uint16_t, uint8_t, void *);
  bool (*action_cb)(uint8_t, void *);
  void (*cleanup_cb)(void *);
  void (*update_cb)(Menu *, void *);
  unsigned int update_interval;
};

/* API */
Menu *menu_create(MenuConfig *config);
void menu_destroy(Menu *menu);

void menu_set_activation_state(MenuConfig *config, int mod_key, int keycode);
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

#endif
