/* menu.h (extended with optional activation callback) */
#ifndef MENU_H
#define MENU_H

#include "x11_focus.h"
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

typedef bool (*MenuActionCallback)(uint8_t keycode, void *user_data);
typedef bool (*MenuActivatesCallback)(uint16_t modifiers, uint8_t keycode,
                                      void *user_data);
typedef void (*MenuCleanupCallback)(void *user_data);

typedef struct {
  uint16_t modifier_mask;
  MenuActivatesCallback activates_cb; // Optional activation callback
  MenuActionCallback action_cb;
  MenuCleanupCallback cleanup_cb;
  void *user_data;
} MenuConfig;

typedef struct MenuContext {
  X11FocusContext *focus_ctx;
  xcb_connection_t *conn;
  MenuConfig config;
  bool active;
  uint16_t active_modifier;
} MenuContext;

MenuContext *menu_create(X11FocusContext *focus_ctx, MenuConfig config);
void menu_destroy(MenuContext *ctx);

bool menu_handle_key_press(MenuContext *ctx, xcb_key_press_event_t *event);
bool menu_handle_key_release(MenuContext *ctx, xcb_key_release_event_t *event);

#endif
