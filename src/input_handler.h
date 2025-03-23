/* input_handler.h - Input handling and event routing */
#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "menu_manager.h"
#include "x11_focus.h"
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

typedef struct {
  uint16_t modifier_mask;
  xcb_connection_t *conn;
  xcb_window_t *root;
  xcb_screen_t *screen;
  xcb_ewmh_connection_t *ewmh;
  X11FocusContext *focus_ctx;
  MenuManager *menu_manager;
  ActivationState *activation_states;
  size_t activation_state_count;
} InputHandler;

/* Initialize input handler with menu manager */
InputHandler *input_handler_create();

void input_handler_setup_x(InputHandler *handler);
/* Clean up input handler resources */
void input_handler_destroy(InputHandler *handler);

/* Process a single input event */
bool input_handler_process_event(InputHandler *handler);

bool input_handler_add_menu(InputHandler *handler, MenuConfig *config);
bool input_handler_remove_menu(InputHandler *handler, Menu *menu);

/* Add activation state */
bool input_handler_add_activation_state(InputHandler *handler,
                                        ActivationState *state);
bool input_handler_handle_activation(InputHandler *handler, uint16_t mod_key,
                                     uint8_t keycode);

/* Run the main event loop */
void input_handler_run(InputHandler *handler);
bool input_handler_handle_event(InputHandler *handler,
                                xcb_generic_event_t *event);

#endif /* INPUT_HANDLER_H */
