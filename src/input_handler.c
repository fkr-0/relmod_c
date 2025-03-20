/* input_handler.c - Input handling implementation */
#include "input_handler.h"
#include "cairo_menu.h"
#include "x11_focus.h"
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>

/* Helper: check if modifier key is being released */
static bool is_modifier_release(uint16_t state, uint8_t keycode) {
  /* Super key */
  if ((state == 0x40 || state == 0x40) && keycode == 133) {
    return true;
  }
  /* Alt key */
  if ((state == 0x08 || state == 0x08) && keycode == 64) {
    return true;
  }
  /* Control key */
  if ((state == 0x04 || state == 0x04) && keycode == 37) {
    return true;
  }
  /* Shift key */
  if ((state == 0x01 || state == 0x01) && keycode == 50) {
    return true;
  }
  return false;
}

/* Private helper function to handle events */
static bool handle_event(InputHandler *handler, xcb_generic_event_t *event) {
  if (!handler || !event)
    return false;

  bool exit = false;
  printf("[DEBUG] Received event type: 0x%x\n", event->response_type & ~0x80);

  switch (event->response_type & ~0x80) {
  case XCB_KEY_PRESS: {
    xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;

    printf("[DEBUG] Key press event - detail: %d, state: 0x%x\n", kp->detail,
           kp->state);

    /* Handle escape key for exit */
    if (kp->detail == 9 && kp->state == 0) {
      printf("[DEBUG] Escape key detected, exiting\n");
      exit = true;
      break;
    }

    /* Handle 'q' for exit */
    if (kp->detail == 24) {
      printf("[DEBUG] Q key detected, exiting\n");
      exit = true;
      break;
    }

    /* Store modifier state */
    if (handler->modifier_mask == 0) {
      if (kp->state != 0) {
        handler->modifier_mask = kp->state;
      } else if (kp->detail == 133) { /* Super */
        handler->modifier_mask = 0x40;
      } else if (kp->detail == 64) { /* Alt */
        handler->modifier_mask = 0x08;
      } else if (kp->detail == 37) { /* Control */
        handler->modifier_mask = 0x04;
      } else if (kp->detail == 50) { /* Shift */
        handler->modifier_mask = 0x01;
      }
      if (handler->modifier_mask != 0) {
        printf("[DEBUG] Modifier mask set to 0x%x\n", handler->modifier_mask);
      }
    }

    /* Forward to menu manager */
    menu_manager_handle_key_press(handler->menu_manager, kp);
    break;
  }

  case XCB_KEY_RELEASE: {
    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
    printf("[DEBUG] Key release event - detail: %d, state: 0x%x\n", kr->detail,
           kr->state);

    /* Check for modifier release */
    if (handler->modifier_mask != 0) {
      if (kr->state != 0) {
        handler->modifier_mask = kr->state;
        printf("[DEBUG] Release: Modifier mask set to 0x%x\n",
               handler->modifier_mask);
      }
    }

    /* Check for terminating conditions */
    if (is_modifier_release(kr->state, kr->detail)) {
      exit = true;
      break;
    }

    /* Forward to menu manager */
    menu_manager_handle_key_release(handler->menu_manager, kr);

    break;
  }

  default:
    printf("[DEBUG] Unhandled event type: 0x%x\n",
           event->response_type & ~0x80);
    break;
  }

  return exit;
}

/* Create input handler */
InputHandler *input_handler_create(xcb_connection_t *conn,
                                   xcb_ewmh_connection_t *ewmh,
                                   xcb_window_t window) {
  if (!conn || !ewmh || window == XCB_NONE) {
    return NULL;
  }

  InputHandler *handler = calloc(1, sizeof(InputHandler));
  if (!handler) {
    return NULL;
  }

  handler->modifier_mask = 0;
  handler->conn = conn;
  handler->focus_ctx = x11_focus_init(conn, ewmh);

  if (!handler->focus_ctx) {
    free(handler);
    return NULL;
  }

  /* Initialize menu manager */
  handler->menu_manager = menu_manager_create(conn, ewmh);
  if (!handler->menu_manager) {
    x11_focus_cleanup(handler->focus_ctx);
    free(handler);
    return NULL;
  }

  /* Set up input focus and grabs */
  x11_set_window_floating(handler->focus_ctx, window);
  if (!x11_grab_inputs(handler->focus_ctx, window)) {
    fprintf(stderr, "Unable to grab inputs\n");
    menu_manager_destroy(handler->menu_manager);
    x11_focus_cleanup(handler->focus_ctx);
    free(handler);
    return NULL;
  }

  return handler;
}

/* Clean up input handler */
void input_handler_destroy(InputHandler *handler) {
  if (!handler) {
    return;
  }

  if (handler->focus_ctx) {
    x11_release_inputs(handler->focus_ctx);
    x11_focus_cleanup(handler->focus_ctx);
  }

  if (handler->menu_manager) {
    menu_manager_destroy(handler->menu_manager);
  }

  free(handler);
}

/* Process a single event */
bool input_handler_process_event(InputHandler *handler) {
  if (!handler)
    return false;

  xcb_generic_event_t *event = xcb_poll_for_event(handler->conn);
  if (!event) {
    return false;
  }

  bool should_exit = handle_event(handler, event);
  free(event);
  return should_exit;
}

/* Run event loop */
void input_handler_run(InputHandler *handler) {
  if (!handler)
    return;

  xcb_generic_event_t *event;
  while ((event = xcb_wait_for_event(handler->conn))) {
    if (handle_event(handler, event)) {
      free(event);
      break;
    }
    free(event);
  }
}

bool input_handler_add_menu(InputHandler *handler, MenuConfig menu_config) {
  if (!handler || !handler->menu_manager) {
    return false;
  }
  Menu *menu = menu_create_from_config(&menu_config);
  if (!menu) {
    return false;
  }
  return menu_manager_register(handler->menu_manager, menu);
}

bool input_handler_remove_menu(InputHandler *handler, MenuConfig menu_config) {
  if (!handler || !handler->menu_manager) {
    return false;
  }
  Menu *menu = menu_create_from_config(&menu_config);
  if (!menu) {
    return false;
  }
  menu_manager_unregister(handler->menu_manager, menu);
  menu_destroy(menu);
  return true;
}
