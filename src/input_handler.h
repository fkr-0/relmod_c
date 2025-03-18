/* input_handler.h */
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "menu.h"
#include "x11_focus.h"
#include <xcb/xcb.h>

#define MAX_MENUS 10

typedef struct {
  uint16_t modifier_mask;
  xcb_connection_t *conn;
  X11FocusContext *focus_ctx;
  MenuContext *menus[MAX_MENUS];
  size_t menu_count;
} InputHandler;

// Create input handler without menus initially
InputHandler *input_handler_create(xcb_connection_t *conn,
                                   xcb_ewmh_connection_t *ewmh,
                                   xcb_window_t window);

// Dynamically add menu configuration
bool input_handler_add_menu(InputHandler *handler, MenuConfig config);

// Run event loop
void input_handler_run(InputHandler *handler);

// Process a single event (for testing)
bool input_handler_process_event(InputHandler *handler);

// Cleanup resources
void input_handler_destroy(InputHandler *handler);

#endif
