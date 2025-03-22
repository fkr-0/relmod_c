/* input_handler.c - Complete and updated Input handling implementation */
#include "input_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef MENU_DEBUG
#define LOG(fmt, ...) fprintf(stderr, "[INPUT] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...) ((void)0)
#endif

static bool is_modifier_release(uint16_t state, uint8_t keycode) {
  return (state == 0x40 && keycode == 133) || // Super
         (state == 0x08 && keycode == 64) ||  // Alt
         (state == 0x04 && keycode == 37) ||  // Ctrl
         (state == 0x01 && keycode == 50);    // Shift
}

InputHandler *input_handler_create(xcb_connection_t *conn,
                                   xcb_ewmh_connection_t *ewmh,
                                   xcb_window_t window) {
  if (!conn || !ewmh || window == XCB_NONE)
    return NULL;

  InputHandler *handler = calloc(1, sizeof(InputHandler));
  if (!handler)
    return NULL;

  handler->conn = conn;
  /* handler->window = window; */
  handler->modifier_mask = 0;
  handler->focus_ctx = x11_focus_init(conn, ewmh);

  if (!handler->focus_ctx)
    goto fail;

  handler->menu_manager = menu_manager_create(conn, ewmh);
  if (!handler->menu_manager)
    goto fail;

  x11_set_window_floating(handler->focus_ctx, window);
  if (!x11_grab_inputs(handler->focus_ctx, window)) {
    fprintf(stderr, "[INPUT] Failed to grab inputs\n");
    goto fail;
  }

  return handler;

fail:
  if (handler->menu_manager)
    menu_manager_destroy(handler->menu_manager);
  if (handler->focus_ctx)
    x11_focus_cleanup(handler->focus_ctx);
  free(handler);
  return NULL;
}

void input_handler_destroy(InputHandler *handler) {
  if (!handler)
    return;
  if (handler->focus_ctx) {
    x11_release_inputs(handler->focus_ctx);
    x11_focus_cleanup(handler->focus_ctx);
  }
  if (handler->menu_manager)
    menu_manager_destroy(handler->menu_manager);
  free(handler);
}

static bool update_callback(Menu *menu, struct timeval *last_update,
                            void *user_data) {
  struct timeval now;
  gettimeofday(&now, NULL);

  if (!menu->active || menu->update_interval <= 0 || !menu->update_cb)
    return true;

  unsigned long elapsed = (now.tv_sec - last_update->tv_sec) * 1000 +
                          (now.tv_usec - last_update->tv_usec) / 1000;
  if (elapsed >= menu->update_interval) {
    menu_trigger_update(menu);
    *last_update = now;
  }
  return true;
}

void input_handler_run(InputHandler *handler) {
  if (!handler)
    return;

  int fd = xcb_get_file_descriptor(handler->conn);
  while (1) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval timeout = {0};
    timeout.tv_sec = 1; // fallback timeout

    int ret = select(fd + 1, &fds, NULL, NULL, &timeout);

    if (ret > 0) {
      xcb_generic_event_t *event;
      while ((event = xcb_poll_for_event(handler->conn))) {
        if (input_handler_handle_event(handler, event)) {
          free(event);
          return;
        }
        free(event);
      }
    }

    menu_manager_foreach(handler->menu_manager, update_callback, NULL);

    if (xcb_connection_has_error(handler->conn)) {
      LOG("X11 connection error detected, exiting loop");
      break;
    }
  }
}

bool input_handler_process_event(InputHandler *handler) {
  if (!handler)
    return false;
  xcb_generic_event_t *event = xcb_poll_for_event(handler->conn);
  if (!event)
    return false;

  bool result = input_handler_handle_event(handler, event);
  free(event);
  return result;
}

bool input_handler_handle_event(InputHandler *handler,
                                xcb_generic_event_t *event) {
  if (!handler || !event)
    return false;

  uint8_t type = event->response_type & ~0x80;
  switch (type) {
  case XCB_KEY_PRESS: {
    xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
    LOG("Key press: code=%u, state=0x%x", kp->detail, kp->state);

    if (kp->detail == 9 || kp->detail == 24)
      return true; // ESC or 'q'

    if (handler->modifier_mask == 0 && kp->state != 0)
      handler->modifier_mask = kp->state;

    return menu_manager_handle_key_press(handler->menu_manager, kp);
  }
  case XCB_KEY_RELEASE: {
    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
    LOG("Key release: code=%u, state=0x%x", kr->detail, kr->state);

    if (handler->modifier_mask == 0 && kr->state != 0)
      handler->modifier_mask = kr->state;

    menu_manager_handle_key_release(handler->menu_manager, kr);
    return is_modifier_release(kr->state, kr->detail);
  }
  default:
    LOG("Unhandled event type: 0x%x", type);
    return false;
  }
}

bool input_handler_add_menu(InputHandler *handler, Menu *menu) {
  if (!handler || !menu)
    return false;
  return menu_manager_register(handler->menu_manager, menu);
}

bool input_handler_remove_menu(InputHandler *handler, Menu *menu) {
  if (!handler || !menu)
    return false;
  menu_manager_unregister(handler->menu_manager, menu);
  return true;
}
