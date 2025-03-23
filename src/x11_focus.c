/* x11_focus.c - Unchanged input handling and X11 window management */

#include "x11_focus.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>

static xcb_atom_t get_atom(xcb_connection_t *conn, const char *name) {
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(conn, 0, strlen(name), name);
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
  if (!reply)
    return XCB_NONE;
  xcb_atom_t atom = reply->atom;
  free(reply);
  return atom;
}

X11FocusContext *x11_focus_init(xcb_connection_t *conn,
                                xcb_ewmh_connection_t *ewmh) {

  X11FocusContext *ctx = calloc(1, sizeof(X11FocusContext));
  ctx->conn = conn;
  ctx->ewmh = ewmh;
  ctx->previous_focus = XCB_NONE;
  return ctx;
}

void x11_focus_cleanup(X11FocusContext *ctx) {
  if (ctx)
    free(ctx);
}

void x11_set_window_floating(X11FocusContext *ctx, xcb_window_t window) {
  xcb_ewmh_connection_t ewmh = *ctx->ewmh;
  xcb_atom_t wm_window_type = ewmh._NET_WM_WINDOW_TYPE;
  xcb_atom_t wm_window_type_dialog = ewmh._NET_WM_WINDOW_TYPE_DIALOG;

  /* xcb_atom_t dialog = ctx->ewmh->_NET_WM_WINDOW_TYPE_DIALOG; */
  xcb_change_property(ctx->conn, XCB_PROP_MODE_REPLACE, window, wm_window_type,
                      XCB_ATOM_ATOM, 32, 1, &wm_window_type_dialog);

  struct {
    uint32_t flags, functions, decorations;
    int32_t input_mode;
    uint32_t status;
  } hints = {2, 0, 0, 0, 0};

  xcb_atom_t motif = get_atom(ctx->conn, "_MOTIF_WM_HINTS");
  xcb_change_property(ctx->conn, XCB_PROP_MODE_REPLACE, window, motif, motif,
                      32, 5, &hints);
}

static void store_current_focus(X11FocusContext *ctx) {
  xcb_get_input_focus_cookie_t cookie = xcb_get_input_focus(ctx->conn);
  xcb_get_input_focus_reply_t *reply =
      xcb_get_input_focus_reply(ctx->conn, cookie, NULL);
  if (reply) {
    ctx->previous_focus = reply->focus;
    free(reply);
  }
}

static void restore_previous_focus(X11FocusContext *ctx) {
  if (ctx->previous_focus != XCB_NONE) {
    xcb_set_input_focus(ctx->conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                        ctx->previous_focus, XCB_CURRENT_TIME);
    xcb_flush(ctx->conn);
    ctx->previous_focus = XCB_NONE;
  }
}

/* static bool wait_for_map_notify(xcb_connection_t *conn, xcb_window_t window,
 */
/*                                 int max_wait_ms) { */
/*   xcb_flush(conn); // Ensure all pending X requests are sent immediately */

/*   struct pollfd pfd; */
/*   pfd.fd = xcb_get_file_descriptor(conn); */
/*   pfd.events = POLLIN; */

/*   int waited = 0; */

/*   while (waited < max_wait_ms) { */
/*     // Poll for events with timeout of 1 ms */
/*     int ret = poll(&pfd, 1, 1); */
/*     if (ret > 0 && (pfd.revents & POLLIN)) { */
/*       xcb_generic_event_t *event; */
/*       while ((event = xcb_poll_for_event(conn))) { */
/*         uint8_t type = event->response_type & ~0x80; */
/*         if (type == XCB_MAP_NOTIFY) { */
/*           xcb_map_notify_event_t *map = (xcb_map_notify_event_t *)event; */
/*           if (map->window == window) { */
/*             free(event); */
/*             return true; // Correct event received */
/*           } */
/*         } */
/*         free(event); */
/*       } */
/*     } else if (ret < 0) { */
/*       perror("poll failed"); */
/*       return false; // Polling error */
/*     } */

/*     waited += 1; // increment waited by 1ms */
/*   } */

/*   // Timeout reached without receiving expected event */
/*   return false; */
/* } */

static bool wait_for_map_notify(xcb_connection_t *conn, xcb_window_t window,
                                int max_wait_ms) {
  xcb_flush(conn); // Ensure all requests sent

  struct pollfd pfd = {.fd = xcb_get_file_descriptor(conn), .events = POLLIN};

  int waited = 0;
  const int interval_ms = 10;

  while (waited < max_wait_ms) {
    int ret = poll(&pfd, 1, interval_ms);
    if (ret > 0 && (pfd.revents & POLLIN)) {
      xcb_generic_event_t *event;
      while ((event = xcb_poll_for_event(conn))) {
        uint8_t type = event->response_type & ~0x80;
        if (type == XCB_MAP_NOTIFY) {
          xcb_map_notify_event_t *map = (xcb_map_notify_event_t *)event;
          if (map->window == window) {
            free(event);
            return true; // Success
          }
        }
        free(event);
      }
    } else if (ret < 0) {
      perror("poll failed");
      return false; // Polling error
    }
    waited += interval_ms;
  }
  return false; // Timeout
}

static bool take_keyboard(X11FocusContext *ctx, xcb_window_t window,
                          int max_attempts) {
  struct timespec delay = {0, 5000000};
  for (int i = 0; i < max_attempts; i++) {
    xcb_grab_keyboard_cookie_t cookie =
        xcb_grab_keyboard(ctx->conn, true, window, XCB_CURRENT_TIME,
                          XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_keyboard_reply_t *reply =
        xcb_grab_keyboard_reply(ctx->conn, cookie, NULL);
    if (reply) {
      if (reply->status == XCB_GRAB_STATUS_SUCCESS) {
        free(reply);
        return true;
      }
      free(reply);
    }
    nanosleep(&delay, NULL);
  }
  return false;
}

static bool take_pointer(X11FocusContext *ctx, xcb_window_t window,
                         int max_attempts) {
  struct timespec delay = {0, 5000000};
  for (int i = 0; i < max_attempts; i++) {
    xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(
        ctx->conn, true, window,
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window, XCB_NONE,
        XCB_CURRENT_TIME);
    xcb_grab_pointer_reply_t *reply =
        xcb_grab_pointer_reply(ctx->conn, cookie, NULL);
    if (reply) {
      if (reply->status == XCB_GRAB_STATUS_SUCCESS) {
        free(reply);
        return true;
      }
      free(reply);
    }
    nanosleep(&delay, NULL);
  }
  return false;
}

bool x11_grab_inputs(X11FocusContext *ctx, xcb_window_t window) {
  store_current_focus(ctx);

  /* xcb_map_window(ctx->conn, window); */
  /* xcb_flush(ctx->conn); */

  /* if (!wait_for_map_notify(ctx->conn, window, 500)) { */
  /*   fprintf(stderr, "[X11] Timeout waiting for MapNotify on window %u\n",
   */
  /*           window); */
  /*   restore_previous_focus(ctx); */
  /*   return false; */
  /* } */
  /* uint32_t values[] = {XCB_EVENT_MASK_STRUCTURE_NOTIFY}; */
  /* xcb_change_window_attributes(ctx->conn, window, XCB_CW_EVENT_MASK, values);
   */
  /* xcb_map_window(ctx->conn, window); */

  /* if (!wait_for_map_notify(ctx->conn, window, 1000)) { */
  /*   fprintf(stderr, "MapNotify event timeout"); */
  /* } */

  if (!take_keyboard(ctx, window, 500)) {
    fprintf(stderr, "[X11] Keyboard grab failed\n");
    restore_previous_focus(ctx);
    return false;
  }

  if (!take_pointer(ctx, window, 500)) {
    fprintf(stderr, "[X11] Pointer grab failed\n");
    xcb_ungrab_keyboard(ctx->conn, XCB_CURRENT_TIME);
    restore_previous_focus(ctx);
    return false;
  }
  xcb_set_input_focus(ctx->conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
                      XCB_CURRENT_TIME);

  xcb_flush(ctx->conn);
  return true;
}

void x11_release_inputs(X11FocusContext *ctx) {
  xcb_ungrab_keyboard(ctx->conn, XCB_CURRENT_TIME);
  xcb_ungrab_pointer(ctx->conn, XCB_CURRENT_TIME);
  restore_previous_focus(ctx);
  xcb_flush(ctx->conn);
}
