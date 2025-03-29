#define _GNU_SOURCE // Required for asprintf
#include "x11_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#define INITIAL_CAPACITY 32
#ifdef MENU_DEBUG
#define LOG_PREFIX "[X11_WIN]"
#endif
#include "log.h"

// Forward declarations
static void get_window_class_name(xcb_connection_t *conn, xcb_window_t window,
                                  char **class_name, char **instance_name);

static xcb_atom_t get_atom(xcb_connection_t *conn, const char *atom_name) {
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(conn, 0, strlen(atom_name), atom_name);
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);

  if (!reply) {
    return XCB_NONE;
  }

  xcb_atom_t atom = reply->atom;
  free(reply);
  return atom;
}

static char *get_window_title(xcb_connection_t *conn, xcb_window_t window) {
  xcb_get_property_cookie_t cookie;
  xcb_get_property_reply_t *reply;
  char *title = NULL;
  xcb_atom_t utf8_string = get_atom(conn, "UTF8_STRING");

  // Try _NET_WM_NAME first (UTF-8)
  xcb_atom_t net_wm_name = get_atom(conn, "_NET_WM_NAME");
  if (net_wm_name != XCB_NONE) {
    cookie =
        xcb_get_property(conn, 0, window, net_wm_name, utf8_string, 0, 1024);
    reply = xcb_get_property_reply(conn, cookie, NULL);

    if (reply && reply->value_len > 0) {
      int len = xcb_get_property_value_length(reply);
      title = malloc(len + 1);
      memcpy(title, xcb_get_property_value(reply), len);
      title[len] = '\0';
      free(reply);
      return title;
    }
    free(reply);
  }

  // Try WM_NAME (ICCCM)
  xcb_get_property_cookie_t name_cookie = xcb_icccm_get_wm_name(conn, window);
  xcb_icccm_get_text_property_reply_t icccm_reply;
  if (xcb_icccm_get_wm_name_reply(conn, name_cookie, &icccm_reply, NULL)) {
    title = strdup((char *)icccm_reply.name);
    xcb_icccm_get_text_property_reply_wipe(&icccm_reply);
    return title;
  }

  // For i3 containers, try to get the actual window title
  char *class_name = NULL;
  char *instance_name = NULL;
  get_window_class_name(conn, window, &class_name, &instance_name);

  if (class_name && strcmp(class_name, "i3-frame") == 0) {
    xcb_query_tree_cookie_t tree_cookie = xcb_query_tree(conn, window);
    xcb_query_tree_reply_t *tree_reply =
        xcb_query_tree_reply(conn, tree_cookie, NULL);

    if (tree_reply) {
      xcb_window_t *children = xcb_query_tree_children(tree_reply);
      int len = xcb_query_tree_children_length(tree_reply);

      if (len > 0) {
        char *child_title = get_window_title(conn, children[0]);
        if (child_title && strcmp(child_title, "<Untitled>") != 0) {
          free(class_name);
          free(instance_name);
          free(tree_reply);
          return child_title;
        }
        free(child_title);
      }
      free(tree_reply);
    }
  }

  free(class_name);
  free(instance_name);
  return strdup("<Untitled>");
}

WindowList *window_list_init(xcb_connection_t *conn) {
  printf("Initializing window list\n");
  WindowList *list = malloc(sizeof(WindowList));
  if (!list)
    return NULL;

  list->windows = malloc(sizeof(X11Window) * INITIAL_CAPACITY);
  if (!list->windows) {
    free(list);
    return NULL;
  }

  list->count = 0;
  list->capacity = INITIAL_CAPACITY;
  window_list_update(list, conn);
  return list;
}

void window_list_free(WindowList *list) {
  if (!list)
    return;

  for (size_t i = 0; i < list->count; i++) {
    free(list->windows[i].title);
    free(list->windows[i].className);
    free(list->windows[i].instance);
    free(list->windows[i].name);
  }

  free(list->windows);
  free(list);
}

static void get_window_class_name(xcb_connection_t *conn, xcb_window_t window,
                                  char **class_name, char **instance_name) {
  xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(conn, window);
  xcb_icccm_get_wm_class_reply_t reply;

  if (xcb_icccm_get_wm_class_reply(conn, cookie, &reply, NULL)) {
    *instance_name = strdup((char *)reply.instance_name);
    *class_name = strdup((char *)reply.class_name);
    xcb_icccm_get_wm_class_reply_wipe(&reply);
  } else {
    *instance_name = strdup("Unknown");
    *class_name = strdup("Unknown");
  }
}

void window_list_update(WindowList *list, xcb_connection_t *conn) {
  printf("Updating window list\n");
  xcb_ewmh_connection_t ewmh;
  xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(conn, &ewmh);

  if (!xcb_ewmh_init_atoms_replies(&ewmh, ewmh_cookies, NULL)) {
    printf("Failed to initialize EWMH atoms\n");
    return;
  }

  xcb_get_property_cookie_t client_list_cookie =
      xcb_ewmh_get_client_list_stacking(&ewmh, 0);

  xcb_ewmh_get_windows_reply_t windows;
  if (!xcb_ewmh_get_client_list_stacking_reply(&ewmh, client_list_cookie,
                                               &windows, NULL)) {
    printf("Failed to get client list stacking\n");
    xcb_ewmh_connection_wipe(&ewmh);
    return;
  }

  xcb_window_t *client_list = windows.windows;
  uint32_t len = windows.windows_len;
  xcb_window_t focused = window_get_focused(conn);

  // Reset window list
  for (size_t i = 0; i < list->count; i++) {
    free(list->windows[i].title);
    free(list->windows[i].className);
    free(list->windows[i].instance);
    free(list->windows[i].name);
  }
  list->count = 0;

  // Ensure capacity
  if (len > list->capacity) {
    size_t new_capacity = len;
    X11Window *new_windows =
        realloc(list->windows, sizeof(X11Window) * new_capacity);
    if (!new_windows) {
      xcb_ewmh_get_windows_reply_wipe(&windows);
      xcb_ewmh_connection_wipe(&ewmh);
      return;
    }
    list->windows = new_windows;
    list->capacity = new_capacity;
  }

  // Fill window list
  for (uint32_t i = 0; i < len; i++) {
    char *class_name = NULL;
    char *instance_name = NULL;
    get_window_class_name(conn, client_list[i], &class_name, &instance_name);

    xcb_window_t window_to_use = client_list[i];
    if (class_name && strcmp(class_name, "i3-frame") == 0) {
      xcb_query_tree_cookie_t tree_cookie =
          xcb_query_tree(conn, client_list[i]);
      xcb_query_tree_reply_t *tree_reply =
          xcb_query_tree_reply(conn, tree_cookie, NULL);

      if (tree_reply) {
        xcb_window_t *container_children = xcb_query_tree_children(tree_reply);
        int container_len = xcb_query_tree_children_length(tree_reply);

        if (container_len > 0) {
          window_to_use = container_children[0];
        }
        free(tree_reply);
      }
    }

    char *title = get_window_title(conn, window_to_use);
    LOG("[%d]: Window title: %s", i, title);
    if (!title || strcmp(title, "<Untitled>") == 0) {
      free(title);
      free(class_name);
      free(instance_name);
      continue;
    }

    if (window_to_use != client_list[i]) {
      free(class_name);
      free(instance_name);
      get_window_class_name(conn, window_to_use, &class_name, &instance_name);
    }

    list->windows[list->count].id = client_list[i];
    list->windows[list->count].focused = (client_list[i] == focused);

    xcb_get_property_cookie_t desktop_cookie =
        xcb_ewmh_get_wm_desktop(&ewmh, client_list[i]);
    uint32_t desktop;
    if (xcb_ewmh_get_wm_desktop_reply(&ewmh, desktop_cookie, &desktop, NULL)) {
      list->windows[list->count].desktop = desktop;
    } else {
      list->windows[list->count].desktop = 0;
    }

    char *desktop_title = NULL;
    if (asprintf(&desktop_title, "[%d] %s", list->windows[list->count].desktop,
                 title) != -1) {
      free(title);
      list->windows[list->count].title = desktop_title;
    } else {
      list->windows[list->count].title = title;
    }

    list->windows[list->count].className = class_name;
    list->windows[list->count].instance = instance_name;
    list->windows[list->count].name = strdup(list->windows[list->count].title);
    list->count++;
  }

  xcb_ewmh_get_windows_reply_wipe(&windows);
  xcb_ewmh_connection_wipe(&ewmh);
}

/* Filter the window list based on the given filter function.
 * The filter function should return true if the window should be included in
 * the filtered list, and false otherwise.
 * Arguments:
 * - list: The window list to filter [WindowList *]
 * - filter: The filter function [WindowFilterFn]
 * Returns:
 * - A new window list containing only the windows that passed the filter
 * [WindowList *]
 * - NULL on error
 * Example:
 *  SubstringFilterData filter_data = {.substring = "Firefox"};
 * WindowList *filtered = window_list_filter(list, window_filter_substring,
 * &filter_data);
 *
 * filtered in the example only contains windows with "Firefox" in the title.
 * The caller is responsible for freeing the returned list.
 */
WindowList *window_list_filter(const WindowList *list, WindowFilterFn filter,
                               const void *filter_data) {
  WindowList *filtered = malloc(sizeof(WindowList));
  if (!filtered)
    return NULL;

  filtered->capacity = list->count > 0 ? list->count : 1;
  filtered->windows = malloc(sizeof(X11Window) * filtered->capacity);
  if (!filtered->windows) {
    free(filtered);
    return NULL;
  }

  filtered->count = 0;

  for (size_t i = 0; i < list->count; i++) {
    if (filter(&list->windows[i], filter_data)) {
      filtered->windows[filtered->count].id = list->windows[i].id;
      filtered->windows[filtered->count].title = strdup(list->windows[i].title);
      filtered->windows[filtered->count].focused = list->windows[i].focused;
      filtered->windows[filtered->count].desktop = list->windows[i].desktop;
      filtered->windows[filtered->count].className =
          strdup(list->windows[i].className);
      filtered->windows[filtered->count].instance =
          strdup(list->windows[i].instance);
      filtered->windows[filtered->count].name = strdup(list->windows[i].name);
      filtered->count++;
    }
  }

  return filtered;
}

bool window_filter_substring(const X11Window *window, const void *data) {
  const SubstringFilterData *filter_data = data;
  return strstr(window->title, filter_data->substring) != NULL;
}

bool window_filter_substrings_any(const X11Window *window, const void *data) {
  const SubstringsFilterData *filter_data = data;
  for (size_t i = 0; i < filter_data->substring_count; i++) {
    if (strstr(window->title, filter_data->substrings[i]) != NULL) {
      return true;
    }
  }
  return false;
}

bool window_filter_substrings_all(const X11Window *window, const void *data) {
  const SubstringsFilterData *filter_data = data;
  for (size_t i = 0; i < filter_data->substring_count; i++) {
    if (strstr(window->title, filter_data->substrings[i]) == NULL) {
      return false;
    }
  }
  return true;
}

void window_focus(xcb_connection_t *conn, xcb_window_t window) {
  xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window,
                      XCB_CURRENT_TIME);
  xcb_flush(conn);
}

void window_raise(xcb_connection_t *conn, xcb_window_t window) {
  uint32_t values[] = {XCB_STACK_MODE_ABOVE};
  xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE, values);
  xcb_flush(conn);
}

uint32_t window_get_desktop(xcb_connection_t *conn, xcb_window_t window) {
  xcb_ewmh_connection_t ewmh;
  // Initialize EWMH atoms (this could be cached for efficiency)
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);

  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    // Clean up on failure
    xcb_ewmh_connection_wipe(&ewmh);
    return 0; // May want to return a special value to indicate error
  }

  // Request the _NET_WM_DESKTOP property for the given window
  xcb_get_property_cookie_t desktop_cookie =
      xcb_ewmh_get_wm_desktop(&ewmh, window);
  uint32_t desktop;
  if (xcb_ewmh_get_wm_desktop_reply(&ewmh, desktop_cookie, &desktop, NULL)) {
    // Clean up the EWMH connection data
    xcb_ewmh_connection_wipe(&ewmh);
    // Check for sticky windows: EWMH defines sticky as 0xFFFFFFFF
    if (desktop == 0xFFFFFFFF) {
      // Handle sticky windows as needed (here we log it and return the value)
      fprintf(stderr, "Warning: Window 0x%X is sticky (on all desktops).\n",
              window);
      return desktop;
    }
    // Return the detected desktop number
    return desktop;
  }

  xcb_ewmh_connection_wipe(&ewmh);
  // If property is not found, consider whether 0 is a valid desktop or if
  // another sentinel should be used.
  return 0;
}

void window_activate(xcb_connection_t *conn, xcb_window_t window) {
  window_focus(conn, window);
  window_raise(conn, window);
  // with ewmh
  xcb_client_message_event_t event;
  xcb_ewmh_connection_t ewmh = {0};
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);

  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    fprintf(stderr, "Cannot initialize EWMH atoms\n");
    xcb_disconnect(conn);
    exit(1);
  }

  event.response_type = XCB_CLIENT_MESSAGE;
  event.window = window;
  event.type = ewmh._NET_ACTIVE_WINDOW;
  event.format = 32;
  event.data.data32[0] = 2; // 2 means the source is a pager
  event.data.data32[1] = XCB_CURRENT_TIME;
  event.data.data32[2] = window;
  event.data.data32[3] = 0;
  LOG("Sending event");

  xcb_send_event(conn, 0, ewmh._NET_WM_WINDOW_TYPE,
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                 (char *)&event);
  xcb_flush(conn);
}

xcb_window_t window_get_focused(xcb_connection_t *conn) {
  xcb_get_input_focus_cookie_t cookie = xcb_get_input_focus(conn);
  xcb_get_input_focus_reply_t *reply =
      xcb_get_input_focus_reply(conn, cookie, NULL);

  if (!reply)
    return XCB_NONE;

  xcb_window_t window = reply->focus;
  free(reply);
  return window;
}

void focus_window(xcb_connection_t *connection, xcb_ewmh_connection_t ewmhh,
                  xcb_window_t window) {
  /* xcb_connection_t *connection = xcb_connect(NULL, NULL); */
  /* if (xcb_connection_has_error(connection)) { */
  /*     fprintf(stderr, "Cannot open display\n"); */
  /*     exit(1); */
  /* } */

  xcb_ewmh_connection_t ewmh;
  LOG("Initializing EWMH");
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(connection, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    fprintf(stderr, "Cannot initialize EWMH atoms\n");
    xcb_disconnect(connection);
    exit(1);
  }

  /* xcb_window_t window = (xcb_window_t)window_id; */

  xcb_client_message_event_t event;
  event.response_type = XCB_CLIENT_MESSAGE;
  event.window = window;
  event.type = ewmh._NET_ACTIVE_WINDOW;
  event.format = 32;
  event.data.data32[0] = 2; // 2 means the source is a pager, 3 means the
  // source is a taskbar, 4 means the source is a user, 1 means the source is
  // a normal application, 5 means the source is a PDA, 6 means the source is a
  // phone
  event.data.data32[1] = XCB_CURRENT_TIME;
  event.data.data32[2] = window;
  event.data.data32[3] = 0; // 0 means a normal application, 1 means a pager, 2
                            // means a taskbar, 3 means a user, 4 means a PDA, 5
  event.data.data32[4] = 0;
  LOG("Sending event");

  xcb_send_event(connection, 0, ewmh._NET_WM_WINDOW_TYPE,
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                 (char *)&event);
  xcb_flush(connection);

  /* xcb_ewmh_connection_wipe(&ewmh); */
  /* xcb_disconnect(connection); */
}
xcb_window_t get_root_window(xcb_connection_t *connection) {
  // Get the first screen from the connection setup
  xcb_screen_t *screen =
      xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
  return screen->root;
}

// Function: switch_to_desktop
// Sends a _NET_CURRENT_DESKTOP client message to the root window to switch
// desktops.
void switch_to_desktop(xcb_connection_t *connection, uint32_t desktop) {
  xcb_client_message_event_t event;
  xcb_ewmh_connection_t ewmh;
  // Initialize EWMH atoms.
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(connection, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    fprintf(stderr, "Cannot initialize EWMH atoms\n");
    xcb_disconnect(connection);
    exit(1);
  }

  // Get the root window
  xcb_window_t root = get_root_window(connection);

  // Prepare the client message for _NET_CURRENT_DESKTOP.
  event.response_type = XCB_CLIENT_MESSAGE;
  event.window = root; // Target is the root window.
  event.type = ewmh._NET_CURRENT_DESKTOP;
  event.format = 32;
  event.data.data32[0] = desktop; // The desktop number to switch to.
  event.data.data32[1] = XCB_CURRENT_TIME;
  event.data.data32[2] = 0;
  event.data.data32[3] = 0;
  event.data.data32[4] = 0;

  // Send the event to the root window using the correct event mask.
  xcb_send_event(connection, 0, root,
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                 (char *)&event);
  xcb_flush(connection);
}

// Function: switch_to_window
// Sends a _NET_ACTIVE_WINDOW client message to the root window to focus a
// window. Then, it determines the window's desktop and calls switch_to_desktop.
void switch_to_window(xcb_connection_t *connection, xcb_window_t window) {
  xcb_client_message_event_t event;
  xcb_ewmh_connection_t ewmh;
  // Initialize EWMH atoms.
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(connection, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    fprintf(stderr, "Cannot initialize EWMH atoms\n");
    xcb_disconnect(connection);
    exit(1);
  }

  // Get the root window from the default screen.
  xcb_window_t root = get_root_window(connection);

  // Prepare the client message for _NET_ACTIVE_WINDOW.
  event.response_type = XCB_CLIENT_MESSAGE;
  // Set the client message target window (typically the window to activate).
  event.window = window;
  event.type = ewmh._NET_ACTIVE_WINDOW;
  event.format = 32;
  event.data.data32[0] = 2; // 2 indicates the source is a pager.
  event.data.data32[1] = XCB_CURRENT_TIME;
  event.data.data32[2] = window;
  event.data.data32[3] = 0;
  event.data.data32[4] = 0;

  // Send the event to the root window (the correct destination).
  xcb_send_event(connection, 0, root,
                 XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                 (char *)&event);
  xcb_flush(connection);

  // Retrieve the desktop number associated with the window.
  // (Replace this with your actual implementation to get the desktop.)
  uint32_t desktop = window_get_desktop(connection, window);
  /* /\* window_get_desktop(connection, window) *\/ 0; // <-- pseudocode */
  switch_to_desktop(connection, desktop);
}

/* void switch_to_window(xcb_connection_t *connection, xcb_window_t window, */
/*                       xcb_window_t root) { */
/*   xcb_client_message_event_t event; */
/*   xcb_ewmh_connection_t ewmh; */
/*   xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(connection, &ewmh);
 */
/*   if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) { */
/*     fprintf(stderr, "Cannot initialize EWMH atoms\n"); */
/*     xcb_disconnect(connection); */
/*     exit(1); */
/*   } */
/*   /\* xcb_window_t root = window_get_focused(connection); *\/ */
/*   event.response_type = XCB_CLIENT_MESSAGE; */
/*   event.window = get_root_window(connection); */
/*   event.type = ewmh._NET_ACTIVE_WINDOW; */
/*   event.format = 32; */
/*   event.data.data32[0] = 2; // 2 means the source is a pager */
/*   event.data.data32[1] = XCB_CURRENT_TIME; */
/*   event.data.data32[2] = window; */
/*   event.data.data32[3] = 0; */
/*   LOG("Sending event"); */

/*   xcb_send_event(connection, 0, ewmh._NET_WM_WINDOW_TYPE, */
/*                  XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | */
/*                      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, */
/*                  (char *)&event); */
/*   xcb_flush(connection); */
/*   uint32_t desktop = window_get_desktop(connection, window); */
/*   switch_to_desktop(connection, window, desktop, root); */
/* } */
/* /\* pub fn focus_window(window_id: u64) { *\/ */
/* /\*     Command::new("wmctrl") *\/ */
/* /\*         .arg("-i") *\/ */
/* /\*         .arg("-R") *\/ */
/* /\*         .arg(format!("{:#X}", window_id)) *\/ */
/* /\*         .output() *\/ */
/* /\*         .expect("failed to execute wmctrl"); *\/ */
/* /\* } *\/ */
/* /\* pub fn switch_to_window(window_id: u64) { *\/ */
/* /\*     Command::new("wmctrl") *\/ */
/* /\*         .arg("-i") *\/ */
/* /\*         .arg("-a") *\/ */
/* /\*         .arg(format!("{:#X}", window_id)) *\/ */
/* /\*         .output() *\/ */
/* /\*         .expect("failed to execute wmctrl"); *\/ */
/* /\* } *\/ */

/* void switch_to_desktop(xcb_connection_t *connection, xcb_window_t window, */
/*                        uint32_t desktop, xcb_window_t root) { */
/*   xcb_ewmh_connection_t ewmh; */
/*   xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(connection, &ewmh);
 */
/*   if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) { */
/*     fprintf(stderr, "Cannot initialize EWMH atoms\n"); */
/*     xcb_disconnect(connection); */
/*     exit(1); */
/*   } */

/*   xcb_client_message_event_t event; */
/*   event.response_type = XCB_CLIENT_MESSAGE; */
/*   event.window = root; */
/*   event.type = ewmh._NET_CURRENT_DESKTOP; */
/*   event.format = 32; */
/*   event.data.data32[0] = desktop; */
/*   event.data.data32[1] = XCB_CURRENT_TIME; */
/*   event.data.data32[2] = 0; */
/*   event.data.data32[3] = 0; */
/*   event.data.data32[4] = 0; */
/*   LOG("Sending event"); */

/*   xcb_send_event(connection, 0, ewmh._NET_WM_DESKTOP, */
/*                  XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | */
/*                      XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, */
/*                  (char *)&event); */
/*   xcb_flush(connection); */
/* } */

/* // get root window */
/* xcb_window_t get_root_window(xcb_connection_t *connection) { */
/*   xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection))
 */
/*                              .data; // Get the screen */
/*   return screen->root; */
/* } */
