#include "window_menu.h"
#include "cairo_menu.h"
#include "menu.h"
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

typedef struct {
  xcb_connection_t *conn;
  WindowList *window_list;
  int selected_index;
  xcb_window_t window;
  cairo_surface_t *surface;
  cairo_t *cr;
  int width;
  int height;
} WindowMenuData;

static void draw_window_menu(WindowMenuData *data) {
  cairo_t *cr = data->cr;

  // Clear background
  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
  cairo_paint(cr);

  // Draw window list
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 14);

  for (size_t i = 0; i < data->window_list->count; i++) {
    if ((int)i == data->selected_index) {
      // Highlight selected item
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.8);
      cairo_rectangle(cr, 0, i * 20, data->width, 20);
      cairo_fill(cr);
      cairo_set_source_rgb(cr, 1, 1, 1);
    } else {
      cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    }

    // Draw window title with desktop number
    cairo_move_to(cr, 10, (i + 1) * 20 - 5);
    cairo_show_text(cr, data->window_list->windows[i].title);

    // Indicate focused window
    if (data->window_list->windows[i].focused) {
      cairo_move_to(cr, data->width - 20, (i + 1) * 20 - 5);
      cairo_show_text(cr, "●");
    }
  }

  cairo_surface_flush(data->surface);
  xcb_flush(data->conn);
}

static bool window_menu_activates(uint16_t modifiers, uint8_t keycode,
                                  void *user_data) {
  (void)keycode;
  WindowMenuData *data = user_data;
  return (modifiers & XCB_MOD_MASK_4);
}

static bool window_menu_action(uint8_t keycode, void *user_data) {
  WindowMenuData *data = user_data;

  switch (keycode) {
  case 111: // Up arrow
    if (data->window_list->count > 0) {
      data->selected_index =
          (data->selected_index - 1 + data->window_list->count) %
          data->window_list->count;
      draw_window_menu(data);
    }
    break;

  case 116: // Down arrow
    if (data->window_list->count > 0) {
      data->selected_index =
          (data->selected_index + 1) % data->window_list->count;
      draw_window_menu(data);
    }
    break;

  case 36: // Enter
    if (data->selected_index >= 0 &&
        data->selected_index < (int)data->window_list->count) {
      xcb_window_t win = data->window_list->windows[data->selected_index].id;
      window_activate(data->conn, win);
      return false; // Close menu
    }
    break;

  case 9:         // Escape
    return false; // Close menu

  default:
    break;
  }
  return true;
}

static void window_menu_cleanup(void *user_data) {
  WindowMenuData *data = user_data;
  if (data) {
    cairo_destroy(data->cr);
    cairo_surface_destroy(data->surface);
    xcb_destroy_window(data->conn, data->window);
    free(data);
  }
}

static xcb_window_t create_window_menu_window(xcb_connection_t *conn,
                                              xcb_window_t parent, int width,
                                              int height) {
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

  // Position menu in top-right corner
  int x = screen->width_in_pixels - width - 20;
  int y = 20;

  uint32_t mask =
      XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
  uint32_t values[] = {0, // background color
                       1, // override redirect
                       XCB_EVENT_MASK_EXPOSURE};

  xcb_window_t window = xcb_generate_id(conn);
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, parent, x, y, width,
                    height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual, mask, values);

  xcb_map_window(conn, window);
  xcb_flush(conn);
  return window;
}

MenuConfig window_menu_create(xcb_connection_t *conn,
                              xcb_window_t parent_window,
                              uint16_t modifier_mask, WindowList *window_list) {
  WindowMenuData *data = calloc(1, sizeof(WindowMenuData));
  data->conn = conn;
  data->window_list = window_list;
  data->selected_index = -1;
  data->width = 400;
  data->height = window_list->count * 20 + 20;

  // Create window
  data->window =
      create_window_menu_window(conn, parent_window, data->width, data->height);

  // Set up Cairo
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_visualtype_t *visual = NULL;

  // Find visual
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter =
        xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        visual = visual_iter.data;
        break;
      }
    }
    if (visual)
      break;
  }

  data->surface = cairo_xcb_surface_create(conn, data->window, visual,
                                           data->width, data->height);
  data->cr = cairo_create(data->surface);

  // Initial draw
  draw_window_menu(data);

  return (MenuConfig){.modifier_mask = modifier_mask,
                      .activates_cb = window_menu_activates,
                      .action_cb = window_menu_action,
                      .cleanup_cb = window_menu_cleanup,
                      .user_data = data};
}

xcb_window_t window_menu_get_selected(void *user_data) {
  WindowMenuData *data = user_data;
  if (data && data->selected_index >= 0 &&
      data->selected_index < (int)data->window_list->count) {
    return data->window_list->windows[data->selected_index].id;
  }
  return XCB_NONE;
}

void window_menu_update_windows(void *user_data) {
  WindowMenuData *data = user_data;
  if (data) {
    window_list_update(data->window_list, data->conn);
    data->height = data->window_list->count * 20 + 20;

    // Resize surface if needed
    cairo_xcb_surface_set_size(data->surface, data->width, data->height);

    // Update window size
    uint32_t values[] = {data->width, data->height};
    xcb_configure_window(data->conn, data->window,
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         values);

    draw_window_menu(data);
  }
}
