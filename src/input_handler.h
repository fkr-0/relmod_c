/* input_handler.h - Input handling and event routing */
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "menu_manager.h"
#include "x11_focus.h"
#include <xcb/xcb.h>

typedef struct {
  uint16_t modifier_mask;
  xcb_connection_t *conn;
  X11FocusContext *focus_ctx;
  MenuManager *menu_manager; /* Replace array with manager */
} InputHandler;

/* Initialize input handler with menu manager */
InputHandler *input_handler_create(xcb_connection_t *conn,
                                   xcb_ewmh_connection_t *ewmh,
                                   xcb_window_t window);

/* Clean up input handler resources */
void input_handler_destroy(InputHandler *handler);

/* Process a single input event */
bool input_handler_process_event(InputHandler *handler);

bool input_handler_add_menu(InputHandler *handler, Menu *menu);
bool input_handler_remove_menu(InputHandler *handler, Menu *menu);
/* Run the main event loop */
void input_handler_run(InputHandler *handler);
bool input_handler_handle_event(InputHandler *handler,
                                xcb_generic_event_t *event);

#endif /* INPUT_HANDLER_H */
