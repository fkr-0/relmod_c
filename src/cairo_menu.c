#include "cairo_menu.h"
#include <cairo/cairo-xcb.h>
#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Menu data to store cairo context and items
typedef struct {
  xcb_connection_t *conn;
  xcb_window_t window;
  cairo_surface_t *surface;
  cairo_t *cr;
  char **items;
  int item_count;
  int selected;
  int width;
  int height;
} CairoMenuData;

// Forward declarations
static void draw_menu(CairoMenuData *data);

// Activation check: activate when modifier pressed
static bool cairo_menu_activates(uint16_t modifiers, uint8_t keycode,
                                 void *user_data) {
  (void)keycode; // Unused
  printf("Modifier mask: %d\n", modifiers);
  printf("Keycode: %d\n", keycode);
  CairoMenuData *data = user_data;
  return (modifiers & XCB_MOD_MASK_4); // Example: Super key
}

// Handle key actions within menu (navigation)
static bool cairo_menu_action(uint8_t keycode, void *user_data) {
  CairoMenuData *data = user_data;
  switch (keycode) {
  case 111: // Up arrow
    data->selected = (data->selected - 1 + data->item_count) % data->item_count;
    draw_menu(data);
    break;
  case 116: // Down arrow
    data->selected = (data->selected + 1) % data->item_count;
    draw_menu(data);
    break;
  case 36: // Enter
    printf("Selected item: %s\n", data->items[data->selected]);
    return false; // Deactivate menu after selection
  default:
    break;
  }
  return true; // Keep menu active otherwise
}

// Cleanup resources
static void cairo_menu_cleanup(void *user_data) {
  CairoMenuData *data = user_data;
  cairo_destroy(data->cr);
  cairo_surface_destroy(data->surface);
  xcb_destroy_window(data->conn, data->window);
  free(data);
}

// Helper: create popup window top-right
static xcb_window_t create_popup(xcb_connection_t *conn, xcb_window_t root,
                                 int w, int h) {
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

  int x = screen->width_in_pixels - w - 20; // Padding 20px
  int y = 20;

  uint32_t mask =
      XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
  uint32_t vals[3] = {0, 1, XCB_EVENT_MASK_EXPOSURE};

  xcb_window_t win = xcb_generate_id(conn);
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, win, root, x, y, w, h, 1,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask,
                    vals);
  xcb_map_window(conn, win);
  xcb_flush(conn);
  return win;
}

// Helper: render menu items
static void draw_menu(CairoMenuData *data) {
  cairo_t *cr = data->cr;

  cairo_set_source_rgba(cr, 0.1, 0.1, 0.1, 0.9);
  cairo_paint(cr);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 14);

  for (int i = 0; i < data->item_count; i++) {
    if (i == data->selected) {
      cairo_set_source_rgb(cr, 0.3, 0.3, 0.8); // Highlight
      cairo_rectangle(cr, 0, i * 20, data->width, 20);
      cairo_fill(cr);
      cairo_set_source_rgb(cr, 1, 1, 1);
    } else {
      cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    }
    cairo_move_to(cr, 10, (i + 1) * 20 - 5);
    cairo_show_text(cr, data->items[i]);
  }

  cairo_surface_flush(data->surface);
  xcb_flush(data->conn);
}

// Public API to create menu
MenuConfig cairo_menu_create(xcb_connection_t *conn, xcb_window_t parent_window,
                             uint16_t modifier_mask, char **items,
                             int item_count) {
  CairoMenuData *data = calloc(1, sizeof(CairoMenuData));
  data->conn = conn;
  data->items = items;
  data->item_count = item_count;
  data->selected = 0;
  data->width = 200;
  data->height = item_count * 20;

  data->window = create_popup(conn, parent_window, data->width, data->height);

  // Set up Cairo - get proper visual type from root visual ID
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
  xcb_visualtype_t *visual = NULL;

  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter =
        xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        visual = visual_iter.data;
        break;
      }
    }
    if (visual != NULL)
      break;
  }

  if (visual == NULL) {
    fprintf(stderr, "Failed to find visual type for root visual\n");
    cairo_menu_cleanup(data);
    return (MenuConfig){0};
  }

  data->surface = cairo_xcb_surface_create(conn, data->window, visual,
                                           data->width, data->height);
  data->cr = cairo_create(data->surface);

  draw_menu(data);

  return (MenuConfig){.modifier_mask = modifier_mask,
                      .activates_cb = cairo_menu_activates,
                      .action_cb = cairo_menu_action,
                      .cleanup_cb = cairo_menu_cleanup,
                      .user_data = data};
}
