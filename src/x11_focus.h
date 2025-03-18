/* x11_focus.h */

#ifndef X11_FOCUS_H
#define X11_FOCUS_H

#include <stdbool.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

typedef struct {
  xcb_connection_t *conn;
  xcb_window_t previous_focus;
  xcb_ewmh_connection_t *ewmh;
} X11FocusContext;

// Initialization and cleanup
X11FocusContext *x11_focus_init(xcb_connection_t *conn,
                                xcb_ewmh_connection_t *ewmh);
void x11_focus_cleanup(X11FocusContext *ctx);

// Window management
void x11_set_window_floating(X11FocusContext *ctx, xcb_window_t window);

// Input handling
bool x11_grab_inputs(X11FocusContext *ctx, xcb_window_t window);
void x11_release_inputs(X11FocusContext *ctx);

#endif
