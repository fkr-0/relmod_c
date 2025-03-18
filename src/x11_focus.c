/* x11_focus.c */

#include "x11_focus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Utility to get atoms
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

// Initialize context
X11FocusContext *x11_focus_init(xcb_connection_t *conn,
                                xcb_ewmh_connection_t *ewmh) {
  X11FocusContext *ctx = calloc(1, sizeof(X11FocusContext));
  ctx->conn = conn;
  ctx->previous_focus = XCB_NONE;
  ctx->ewmh = ewmh;
  return ctx;
}

// Cleanup context
void x11_focus_cleanup(X11FocusContext *ctx) {
  if (ctx)
    free(ctx);
}

// Set window as floating/dialog
void x11_set_window_floating(X11FocusContext *ctx, xcb_window_t window) {
  xcb_atom_t wm_window_type_dialog = ctx->ewmh->_NET_WM_WINDOW_TYPE_DIALOG;
  xcb_change_property(ctx->conn, XCB_PROP_MODE_REPLACE, window,
                      ctx->ewmh->_NET_WM_WINDOW_TYPE, XCB_ATOM_ATOM, 32, 1,
                      &wm_window_type_dialog);

  struct {
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
  } hints = {2, 0, 0, 0, 0};

  xcb_atom_t motif_wm_hints = get_atom(ctx->conn, "_MOTIF_WM_HINTS");
  xcb_change_property(ctx->conn, XCB_PROP_MODE_REPLACE, window, motif_wm_hints,
                      motif_wm_hints, 32, 5, &hints);
}

// Store current focus
static void store_current_focus(X11FocusContext *ctx) {
  xcb_get_input_focus_cookie_t cookie = xcb_get_input_focus(ctx->conn);
  xcb_get_input_focus_reply_t *reply =
      xcb_get_input_focus_reply(ctx->conn, cookie, NULL);
  if (reply) {
    ctx->previous_focus = reply->focus;
    free(reply);
  }
}

// Restore previous focus
static void restore_previous_focus(X11FocusContext *ctx) {
  if (ctx->previous_focus != XCB_NONE) {
    xcb_set_input_focus(ctx->conn, XCB_INPUT_FOCUS_POINTER_ROOT,
                        ctx->previous_focus, XCB_CURRENT_TIME);
    xcb_flush(ctx->conn);
    ctx->previous_focus = XCB_NONE;
  }
}

// Grab keyboard
static bool take_keyboard(X11FocusContext *ctx, xcb_window_t window,
                          int max_attempts) {
  struct timespec delay = {0, 1000000}; // 1ms
  for (int i = 0; i < max_attempts; i++) {
    xcb_grab_keyboard_cookie_t cookie =
        xcb_grab_keyboard(ctx->conn, true, window, XCB_CURRENT_TIME,
                          XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_keyboard_reply_t *reply =
        xcb_grab_keyboard_reply(ctx->conn, cookie, NULL);
    if (reply && reply->status == XCB_GRAB_STATUS_SUCCESS) {
      free(reply);
      return true;
    }
    free(reply);
    nanosleep(&delay, NULL);
  }
  return false;
}

// Grab pointer
static bool take_pointer(X11FocusContext *ctx, xcb_window_t window,
                         int max_attempts) {
  struct timespec delay = {0, 1000000}; // 1ms
  for (int i = 0; i < max_attempts; i++) {
    xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(
        ctx->conn, true, window,
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window, XCB_NONE,
        XCB_CURRENT_TIME);
    xcb_grab_pointer_reply_t *reply =
        xcb_grab_pointer_reply(ctx->conn, cookie, NULL);
    if (reply && reply->status == XCB_GRAB_STATUS_SUCCESS) {
      free(reply);
      return true;
    }
    free(reply);
    nanosleep(&delay, NULL);
  }
  return false;
}

// Public input grab method
bool x11_grab_inputs(X11FocusContext *ctx, xcb_window_t window) {
  store_current_focus(ctx);

  if (!take_keyboard(ctx, window, 100)) {
    restore_previous_focus(ctx);
    fprintf(stderr, "Keyboard grab failed\n");
    return false;
  }

  if (!take_pointer(ctx, window, 100)) {
    xcb_ungrab_keyboard(ctx->conn, XCB_CURRENT_TIME);
    restore_previous_focus(ctx);
    fprintf(stderr, "Pointer grab failed\n");
    return false;
  }

  xcb_set_input_focus(ctx->conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
                      XCB_CURRENT_TIME);
  xcb_flush(ctx->conn);
  return true;
}

// Public input release method
void x11_release_inputs(X11FocusContext *ctx) {
  xcb_ungrab_keyboard(ctx->conn, XCB_CURRENT_TIME);
  xcb_ungrab_pointer(ctx->conn, XCB_CURRENT_TIME);
  restore_previous_focus(ctx);
  xcb_flush(ctx->conn);
}
