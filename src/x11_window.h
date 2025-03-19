#ifndef X11_WINDOW_H
#define X11_WINDOW_H

#include "menu.h"
#include <cairo/cairo-xcb.h>
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>

typedef struct {
  xcb_window_t id;
  char *title;
  char *className;
  char *instance;
  char *name;
  bool focused;
  uint32_t desktop;
} X11Window;

typedef struct {
  X11Window *windows;
  size_t count;
  size_t capacity;
} WindowList;

typedef bool (*WindowFilterFn)(const X11Window *window, const void *data);

typedef struct {
  const char *substring;
} SubstringFilterData;

// Initialize/free window list
WindowList *window_list_init(xcb_connection_t *conn);
void window_list_free(WindowList *list);

// Update window list
void window_list_update(WindowList *list, xcb_connection_t *conn);

// Filter operations
WindowList *window_list_filter(const WindowList *list, WindowFilterFn filter,
                               const void *filter_data);
bool window_filter_substring(const X11Window *window, const void *data);

// Window operations
void window_focus(xcb_connection_t *conn, xcb_window_t window);
void window_raise(xcb_connection_t *conn, xcb_window_t window);
void window_activate(xcb_connection_t *conn, xcb_window_t window);
xcb_window_t window_get_focused(xcb_connection_t *conn);

#endif // X11_WINDOW_H
