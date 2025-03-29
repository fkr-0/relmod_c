#include "cairo_menu_render.h"
#include "x11_window.h"
#include <cairo/cairo-xcb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MENU_DEBUG
#define LOG_PREFIX "[CAIRO_MENU_RENDER]"
#endif
#include "log.h"
int get_window_absolute_geometry(xcb_connection_t *conn, xcb_window_t window) {
  int x = 0, y = 0, width = 0, height = 0;

  // Request the window's geometry (x, y, width, height) relative to its parent.
  xcb_get_geometry_cookie_t geo_cookie = xcb_get_geometry(conn, window);
  xcb_get_geometry_reply_t *geo_reply =
      xcb_get_geometry_reply(conn, geo_cookie, NULL);
  /* if (!geo_reply) { */
  /*   // If reply is NULL, the geometry couldn't be retrieved; return default
   */
  /*   // geometry. */
  /*   return geom; */
  /* } */
  /* x = geo_reply->x; */
  /* y = geo_reply->y; */
  /* width = geo_reply->width; */
  /* height = geo_reply->height; */
  /* int rx = geo_reply->root; */
  /* int ry = geo_reply->root; */
  /* int rw = 0; */
  /* int rh = 0; */
  /* /\* xcb_window_t rw = geo_reply->root; *\/ */
  /* /\* int rh = geo_reply->root->height; *\/ */
  /* LOG("Root geometry: x=%d, y=%d, width=%d, height=%d", rx, ry, rw, rh); */
  /* LOG("Window geometry: x=%d, y=%d, width=%d, height=%d", x, y, width,
   * height); */
  // Translate the window's (0,0) coordinate to the root window coordinates.
  // This gives us the absolute position of the window.
  xcb_translate_coordinates_cookie_t tcookie =
      xcb_translate_coordinates(conn, window, geo_reply->root, 0, 0);
  xcb_translate_coordinates_reply_t *treply =
      xcb_translate_coordinates_reply(conn, tcookie, NULL);
  if (treply) {
    x = treply->dst_x; // Absolute x position relative to the root window.
    y = treply->dst_y; // Absolute y position relative to the root window.
    width = geo_reply->width;
    height = geo_reply->height;
    LOG("Window absolute geometry: x=%d, y=%d, width=%d, height=%d", x, y,
        width, height);
    free(treply);
  }

  // Set the width and height as reported by the geometry reply.
  free(geo_reply);
  return x > 1900 ? 1 : 0;
}

static int get_desktop_x(xcb_connection_t *conn) {
  // Get the active window ID
  xcb_window_t active_window = window_get_focused(conn);
  get_window_absolute_geometry(conn, active_window);
  if (active_window == XCB_NONE) {
    return -1;
  }

  // Get the geometry of the active window
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, active_window);
  xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, NULL);
  if (!reply) {
    return -1;
  }

  // Calculate the x-coordinate of the active window
  int desktop_x = reply->x;
  free(reply);
  return desktop_x;
}
static int get_active_window_top_right_corner(xcb_connection_t *conn) {
  // Get the active window ID
  xcb_window_t active_window = window_get_focused(conn);
  if (active_window == XCB_NONE) {
    return -1;
  }

  // Get the geometry of the active window
  xcb_get_geometry_cookie_t cookie = xcb_get_geometry(conn, active_window);
  xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply(conn, cookie, NULL);
  if (!reply) {
    return -1;
  }

  // Calculate the top-right corner of the active window
  int top_right_x = reply->x; // + reply->width;
  /* int top_right_y = reply->y; */
  free(reply);
  return top_right_x;
}
/* Filename: cairo_menu_render_improvements.c */
/* This file contains improved rendering functions for our Cairo-based UI menu.
   Enhancements include:
   - Anti-aliasing for smooth edges.
   - Gradient background for a modern look.
   - Rounded corners and drop shadows for selected items.
   - Optional fade effect support for animation.
*/
// Function to set the window sticky by changing _NET_WM_STATE property.
// This makes the window appear on all desktops.
static void set_window_sticky(xcb_connection_t *conn, xcb_window_t window) {
  // Get _NET_WM_STATE atom.
  xcb_intern_atom_cookie_t cookie_state =
      xcb_intern_atom(conn, 0, strlen("_NET_WM_STATE"), "_NET_WM_STATE");
  xcb_intern_atom_reply_t *reply_state =
      xcb_intern_atom_reply(conn, cookie_state, NULL);

  // Get _NET_WM_STATE_STICKY atom.
  xcb_intern_atom_cookie_t cookie_sticky = xcb_intern_atom(
      conn, 0, strlen("_NET_WM_STATE_STICKY"), "_NET_WM_STATE_STICKY");
  xcb_intern_atom_reply_t *reply_sticky =
      xcb_intern_atom_reply(conn, cookie_sticky, NULL);

  if (reply_state && reply_sticky) {
    // Set the _NET_WM_STATE property on the window to include
    // _NET_WM_STATE_STICKY. XCB_ATOM_ATOM is used as the property type.
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, reply_state->atom,
                        XCB_ATOM_ATOM, 32, 1, &reply_sticky->atom);
  }
  free(reply_state);
  free(reply_sticky);
}

/*-------------------------*/
/* Utility Drawing Helpers */
/*-------------------------*/

/* Draw a rounded rectangle on the given Cairo context.
   x, y: top-left coordinates.
   width, height: dimensions of the rectangle.
   radius: the corner radius.
*/
static void draw_rounded_rectangle(cairo_t *cr, double x, double y,
                                   double width, double height, double radius) {
  // Start a new sub-path for the rounded rectangle
  cairo_new_sub_path(cr);
  cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI / 2, 0);
  cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI / 2);
  cairo_arc(cr, x + radius, y + height - radius, radius, M_PI / 2, M_PI);
  cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
  cairo_close_path(cr);
}

/* Create a vertical gradient background.
   This function creates a subtle gradient from a slightly darker top to the
   base color.
*/
static void draw_gradient_background(cairo_t *cr, double width, double height,
                                     const double color[4]) {
  cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, 0, height);
  // Darker tone at the top for a slight depth effect
  cairo_pattern_add_color_stop_rgba(pattern, 0, color[0] * 0.8, color[1] * 0.8,
                                    color[2] * 0.8, color[3]);
  cairo_pattern_add_color_stop_rgba(pattern, 1, color[0], color[1], color[2],
                                    color[3]);
  cairo_set_source(cr, pattern);
  cairo_paint(cr);
  cairo_pattern_destroy(pattern);
}

/* Draw a drop shadow for a rectangle.
   This simulates a shadow by drawing an offset rounded rectangle with
   transparency.
*/
static void draw_drop_shadow(cairo_t *cr, double x, double y, double width,
                             double height, double radius, double shadow_offset,
                             const double shadow_color[4]) {
  cairo_save(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, shadow_color[0], shadow_color[1], shadow_color[2],
                        shadow_color[3]);
  draw_rounded_rectangle(cr, x + shadow_offset, y + shadow_offset, width,
                         height, radius);
  cairo_fill(cr);
  cairo_restore(cr);
}

/* Draw a highlight effect for a rectangle.
   This function creates a subtle highlight effect by drawing a gradient-filled
   rounded rectangle.
*/
static void draw_highlight_effect(cairo_t *cr, double x, double y, double width,
                                  double height, double radius,
                                  const double highlight_color[4]) {
  cairo_save(cr);
  cairo_translate(cr, x, y);
  cairo_new_path(cr);
  draw_rounded_rectangle(cr, 0, 0, width, height, radius);
  cairo_clip(cr);
  cairo_pattern_t *pattern = cairo_pattern_create_linear(0, 0, 0, height);
  cairo_pattern_add_color_stop_rgba(
      pattern, 0, highlight_color[0] * 1.2, highlight_color[1] * 1.2,
      highlight_color[2] * 1.2, highlight_color[3]);
  cairo_pattern_add_color_stop_rgba(pattern, 1, highlight_color[0],
                                    highlight_color[1], highlight_color[2],
                                    highlight_color[3]);
  cairo_set_source(cr, pattern);
  cairo_paint(cr);
  cairo_pattern_destroy(pattern);
  cairo_restore(cr);
}

/*-----------------------------*/
/* Improved Rendering Routines */
/*-----------------------------*/

/* Clear the menu background using a gradient effect and smooth anti-aliasing.
 */
void cairo_menu_render_clear(CairoMenuData *data, const MenuStyle *style) {
  cairo_t *cr = data->render.cr;
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  draw_gradient_background(cr, data->render.width, data->render.height,
                           style->background_color);
}

/* Render the title with an enhanced font and anti-aliased text.
   The title is slightly larger and bolder.
*/
void cairo_menu_render_title(CairoMenuData *data, const char *title,
                             const MenuStyle *style) {
  cairo_t *cr = data->render.cr;
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  // Use a bold face for the title
  cairo_select_font_face(cr, style->font_face, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, style->font_size * 1.1);
  cairo_set_source_rgba(cr, style->text_color[0], style->text_color[1],
                        style->text_color[2], style->text_color[3]);
  double x = style->padding;
  double y = style->padding + style->font_size;
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, title);
}

/* Render an individual menu item.
   For a selected item, draw a drop shadow and a gradient-filled rounded
   rectangle, then render the text in white.
*/
void cairo_menu_render_item(CairoMenuData *data, const MenuItem *item,
                            const MenuStyle *style, bool is_selected,
                            double y_position) {
  cairo_t *cr = data->render.cr;
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

  double item_width = data->render.width - style->padding * 2;
  double x = style->padding;
  double radius = 6.0; // Rounded corner radius for modern UI

  // add nuance to the selected item
  double nuance_intensity = 0.2;
  double nuance_color[4] = {0.3, 0.3, 0.3, 0.1}; // Semi-transparent black
  draw_highlight_effect(cr, x, y_position, item_width,
                        style->item_height - style->padding, radius,
                        nuance_color);
  cairo_menu_render_set_opacity(data, 0.1);
  if (is_selected) {
    // Draw a subtle drop shadow to give depth
    double shadow_offset = 3.0;
    double shadow_color[4] = {0, 0, 0, 0.4}; // Semi-transparent black
    draw_drop_shadow(cr, x, y_position, item_width,
                     style->item_height - style->padding, radius, shadow_offset,
                     shadow_color);

    // Draw the selected background with a gradient inside a rounded
    // rectangle
    cairo_save(cr);
    cairo_translate(cr, x, y_position);
    cairo_new_path(cr);
    draw_rounded_rectangle(cr, 0, 0, item_width,
                           style->item_height - style->padding, radius);
    cairo_clip(cr);
    cairo_pattern_t *pattern =
        cairo_pattern_create_linear(0, 0, 0, style->item_height);
    cairo_pattern_add_color_stop_rgba(
        pattern, 0, style->highlight_color[0] * 1.2,
        style->highlight_color[1] * 1.2, style->highlight_color[2] * 1.2,
        style->highlight_color[3]);
    cairo_pattern_add_color_stop_rgba(
        pattern, 1, style->highlight_color[0], style->highlight_color[1],
        style->highlight_color[2], style->highlight_color[3]);
    cairo_set_source(cr, pattern);
    cairo_paint(cr);
    cairo_pattern_destroy(pattern);
    cairo_restore(cr);

    // Render the text in white to improve contrast on the highlighted
    // background.
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
  } else {
    cairo_set_source_rgba(cr, style->text_color[0], style->text_color[1],
                          style->text_color[2], style->text_color[3]);
  }

  cairo_move_to(cr, x + style->padding,
                y_position + style->padding + style->font_size);
  cairo_show_text(cr, item->label);
}

/*-------------------------*/
/* Optional Animation Hook */
/*-------------------------*/

/* This function can be called during your animation update to apply a fade
   effect. progress: a value between 0.0 (completely transparent) and 1.0 (fully
   opaque). Call this after drawing the full menu to composite the fade effect.
   Usage:
     - During fade-in, gradually increase progress from 0.0 to 1.0.
     - For slide animations, adjust the translation (using cairo_translate)
       before rendering items.
    This modular approach lets you mix and match various effects.

   Example:
     void my_animation_update(CairoMenuData *data, double progress) {
       cairo_menu_render_clear(data, &style);
       cairo_menu_render_title(data, "My Menu", &style);
       for (size_t i = 0; i < item_count; i++) {
         cairo_menu_render_item(data, &items[i], &style, i == selected_index,
                                y_position);
       }
       cairo_menu_render_apply_fade(data, progress);
     }

    This function should be called after rendering the full menu to apply the
    fade effect.
*/
void cairo_menu_render_apply_fade(CairoMenuData *data, double progress) {
  cairo_t *cr = data->render.cr;
  cairo_paint_with_alpha(cr, progress);
}

/*
  Integrate these functions with your existing animation code.
  For example:
    - During fade-in, gradually increase progress from 0.0 to 1.0.
    - For slide animations, adjust the translation (using cairo_translate)
      before rendering items.
  This modular approach lets you mix and match various effects.
*/
/* Create menu window */
/* static xcb_window_t create_window(xcb_connection_t *conn, xcb_window_t
 * parent, */
/*                                   X11FocusContext *ctx, xcb_screen_t *screen,
 */
/*                                   int width, int height) { */

/*     int x = screen->width_in_pixels - width - 20; // Padding 20px */
/*     int y = 20; */
/*     xcb_window_t window = xcb_generate_id(conn); */
/*     // printf("Window ID: %d\n", window); */

/*     uint32_t mask = */
/*         XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK; */
/*     uint32_t vals[3] = {0, 1, XCB_EVENT_MASK_EXPOSURE}; */

/*     /\* uint32_t mask = *\/ */
/*     /\*     XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK; *\/ */
/*     /\* uint32_t value_list[] = {screen->black_pixel, *\/ */
/*     /\*                          XCB_EVENT_MASK_KEY_PRESS | *\/ */
/*     /\*                              XCB_EVENT_MASK_FOCUS_CHANGE}; *\/ */
/*     xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, x, y,
 */
/*                       width, height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT, */
/*                       screen->root_visual, mask, vals); */
/*     x11_set_window_floating(ctx, window); */

/*     LOG("Creating window"); */
/*     /\* xcb_screen_t *screen = */
/*   xcb_setup_roots_iterator(xcb_get_setup(conn)).data; */
/*      *\/ */

/*     /\* uint32_t values[3]; *\/ */
/*     /\* uint32_t mask = *\/ */
/*     /\*     XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
 */
/*      *\/ */

/*     /\* values[0] = screen->black_pixel; *\/ */
/*     /\* values[1] = 1; /\\* Override redirect *\\/ *\/ */
/*     /\* values[2] = XCB_EVENT_MASK_EXPOSURE; *\/ */

/*     /\* xcb_window_t window = xcb_generate_id(conn); *\/ */
/*     /\* xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, parent, 0, 0,
 */
/*   /\\* x, */
/*      * y *\\/ *\/ */
/*     /\*                   width, height, /\\* width, height *\\/ *\/ */
/*     /\*                   0,             /\\* border width *\\/ *\/ */
/*     /\*                   XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
 */
/*   mask, */
/*      *\/ */
/*     /\*                   values); *\/ */

/*     LOG("Created window: %u", window); */
/*     return window; */
/* } */
static xcb_window_t create_window(xcb_connection_t *conn, xcb_window_t parent,
                                  X11FocusContext *ctx, xcb_screen_t *screen,
                                  int width, int height) {

  // Default positioning: top-right of the screen with 20px padding.
  /* int x = screen->width_in_pixels - width - 20; */
  int x = get_active_window_top_right_corner(conn); //- width - 20;
  int y = 30;

  // Generate new window ID.
  xcb_window_t window = xcb_generate_id(conn);

  // Set window attributes: background pixel, override redirect, and event mask.
  uint32_t mask =
      XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
  uint32_t vals[3] = {0, 1, XCB_EVENT_MASK_EXPOSURE};

  // Create the window on the root window.
  xcb_create_window(conn, XCB_COPY_FROM_PARENT, window, screen->root, x, y,
                    width, height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual, mask, vals);

  // Optionally, if you have a function to set floating, call it:
  // x11_set_window_floating(ctx, window);

  // Mark the window as sticky so that it appears on all desktops.
  set_window_sticky(conn, window);

  // Debug output.
  printf("Created sticky window: %u at position (%d, %d)\n", window, x, y);
  return window;
}

/* Initialize rendering */

bool cairo_menu_render_init(CairoMenuData *data, xcb_connection_t *conn,
                            xcb_screen_t *screen, xcb_window_t parent,
                            X11FocusContext *ctx, xcb_visualtype_t *visual) {
  CairoMenuRenderData *render = &data->render;
  LOG("Initializing rendering: conn=%p, screen=%p, parent=%u, ctx=%p, "
      "visual=%p",
      conn, screen, parent, ctx, visual);

  /* Create initial window */
  render->width = 400; /* Default size */
  render->height = 300;
  render->window =
      create_window(conn, parent, ctx, screen, render->width, render->height);
  LOG("Creating window: %d", render->window); // Add debug print
  if (render->window == XCB_NONE) {
    // printf("Failed to create window\n");
    return false;
  }

  /* Create Cairo surface */
  // printf("Creating Cairo surface\n");
  render->surface = cairo_xcb_surface_create(conn, render->window, visual,
                                             render->width, render->height);
  // printf("Cairo surface created: %p, status=%d\n", render->surface,
  //      cairo_surface_status(render->surface));
  if (cairo_surface_status(render->surface) != CAIRO_STATUS_SUCCESS) {
    // printf("Failed to create Cairo surface\n");
    xcb_destroy_window(conn, render->window);
    render->window = XCB_NONE; // Set to XCB_NONE after destruction
    return false;
  }

  /* Create Cairo context */
  // printf("Creating Cairo context\n");
  render->cr = cairo_create(render->surface);
  // printf("Cairo context created: %p, status=%d\n", render->cr,
  // cairo_status(render->cr));
  if (cairo_status(render->cr) != CAIRO_STATUS_SUCCESS) {
    // printf("Failed to create Cairo context\n");
    cairo_surface_destroy(render->surface);
    xcb_destroy_window(conn, render->window);
    render->window = XCB_NONE; // Set to XCB_NONE after destruction
    return false;
  }

  render->needs_redraw = true;
  // Usage from here:
  /* cairo_set_source_rgb(render->cr, 1.0, 1.0, 1.0); */
  /* cairo_paint(render->cr); */
  /* cairo_surface_flush(render->surface); */

  LOG("Rendering initialized successfully\n");
  return true;
}

/* Cleanup rendering resources */
void cairo_menu_render_cleanup(CairoMenuData *data) {
  LOG("Cleaning up rendering resources");
  CairoMenuRenderData *render = &data->render;

  if (render->cr) {
    cairo_destroy(render->cr);
    render->cr = NULL;
  }

  if (render->surface) {
    cairo_surface_destroy(render->surface);
    render->surface = NULL;
  }

  // printf("Cleaning up window: %d\n", render->window); // Add debug print
  // printf("test null: %p\n", NULL);                    // Add debug print

  if (render->window != XCB_NONE) { // Check if window is valid
    if (data->conn) {
      // test data->conn to prevent segmentation fault,
      // only destroy if connection is initialized
      // printf("Checking connection: %p\n", data->conn); // Add debug
      // print

      int connection_error = xcb_connection_has_error(data->conn);
      if (connection_error == 0) { // 0 means connection is healthy
        // printf("No error: 1Destroying window: %d using connection:
        // %p\n",
        //       render->window,
        //       data->conn); // Add debug print
        xcb_destroy_window(data->conn, render->window);

        // printf("2Destroyed window: %d using connection: %p\n",
        // render->window,
        //       data->conn); // Add debug print

      } else {
        // printf("Connection error: %d\n",
        //       connection_error); // Add debug print
      }
    }
    render->window = XCB_NONE; // Set to XCB_NONE after destruction
  }
}

/* Window management */
/* void cairo_menu_render_show(CairoMenuData *data) { */
/*   xcb_map_window(data->conn, data->render.window); */
/*   xcb_flush(data->conn); */
/* } */

void cairo_menu_render_show(CairoMenuData *data) {
  if (!data) {
    fprintf(stderr, "Error: cairo_menu_render_show called with NULL data\n");
    return;
  }

  LOG("Showing menu window (conn=%p, window=%u)", data->conn,
      data->render.window);
  // Verify XCB connection and window validity
  if (!data->conn || data->render.window == XCB_NONE) {
    fprintf(stderr,
            "Error: Invalid XCB connection or window (conn=%p, window=%u)\n",
            data->conn, data->render.window);
    return;
  }

  // Map the window to make it visible
  xcb_map_window(data->conn, data->render.window);

  // Force XCB to flush the request to X server immediately
  xcb_flush(data->conn);
  LOG("Menu window shown");

  // Request initial redraw explicitly
  cairo_menu_render_request_update(data);

  // Immediately render once
  if (cairo_menu_render_needs_update(data)) {
    cairo_menu_render_begin(data);
    cairo_menu_render_clear(data, &data->menu->config.style);
    cairo_menu_render_title(data, data->menu->config.title,
                            &data->menu->config.style);
    cairo_menu_render_items(data, data->menu);
    cairo_menu_render_end(data);
  }
  int x_pad = 20;
  int y_pad = 30;
  int height_pad = 0;
  int width_pad = 20;
  int line_height = 42;
  int char_width = 9;
  int min_width = 200; /* Default size */
  xcb_screen_t *screen =
      xcb_setup_roots_iterator(xcb_get_setup(data->conn)).data;
  /* int x = screen->width_in_pixels - width - 20; // Padding 20px */
  int x =
      get_window_absolute_geometry(data->conn, window_get_focused(data->conn));
  x = (x * 1920) + x_pad;
  int y = y_pad;
  int num_items = data->menu->config.item_count;
  int height = ((1 + num_items) * 42) + (2 * height_pad);
  for (int i = 0; i < num_items; i++) {
    int item_width = strlen(data->menu->config.items[i].label) * 9;
    if (item_width > min_width) {
      min_width = item_width;
    }
  }
  min_width += (2 * width_pad);
  xcb_configure_window(data->conn, data->render.window,
                       XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                           XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                       (const uint32_t[]){x, y, min_width, height});
  cairo_menu_render_resize(data, min_width, height);
}

void cairo_menu_render_hide(CairoMenuData *data) {
  xcb_unmap_window(data->conn, data->render.window);
  xcb_flush(data->conn);
}

void cairo_menu_render_resize(CairoMenuData *data, int width, int height) {
  CairoMenuRenderData *render = &data->render;

  if (width == render->width && height == render->height) {
    return;
  }

  render->width = width;
  render->height = height;

  /* Update window size */
  uint32_t values[2] = {(uint32_t)width, (uint32_t)height};
  xcb_configure_window(data->conn, render->window,
                       XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                       values);

  /* Update surface size */
  cairo_xcb_surface_set_size(render->surface, width, height);
  render->needs_redraw = true;
}

/* Rendering operations */
void cairo_menu_render_begin(CairoMenuData *data) {
  // printf("Rendering begin\n");
  cairo_save(data->render.cr);
}

void cairo_menu_render_end(CairoMenuData *data) {
  // printf("Rendering end\n");
  cairo_restore(data->render.cr);
  cairo_surface_flush(data->render.surface);
  xcb_flush(data->conn);
}

/* void cairo_menu_render_clear(CairoMenuData *data, const MenuStyle *style) {
 */
/*   printf("Rendering clear\n"); */
/*   cairo_t *cr = data->render.cr; */

/*   cairo_set_source_rgba(cr, style->background_color[0], */
/*                         style->background_color[1],
 * style->background_color[2], */
/*                         style->background_color[3]); */
/*   cairo_paint(cr); */
/* } */

/* Menu content rendering */
/* void cairo_menu_render_title(CairoMenuData *data, const char *title, */
/*                              const MenuStyle *style) { */
/*   printf("Rendering title: %s\n", title); */
/*   cairo_t *cr = data->render.cr; */

/*   cairo_set_source_rgba(cr, style->text_color[0], style->text_color[1], */
/*                         style->text_color[2], style->text_color[3]); */

/*   cairo_move_to(cr, style->padding, style->padding + style->font_size); */
/*   cairo_show_text(cr, title); */
/* } */

/* void cairo_menu_render_item(CairoMenuData *data, const MenuItem *item, */
/*                             const MenuStyle *style, bool is_selected, */
/*                             double y_position) { */
/*   printf("Rendering item: %s\n", item->label); */
/*   cairo_t *cr = data->render.cr; */

/*   /\* Draw selection highlight *\/ */
/*   if (is_selected) { */
/*     cairo_set_source_rgba(cr, style->highlight_color[0], */
/*                           style->highlight_color[1],
 * style->highlight_color[2], */
/*                           style->highlight_color[3]); */
/*     cairo_rectangle(cr, 0, y_position, data->render.width,
 * style->item_height); */
/*     cairo_fill(cr); */
/*     cairo_set_source_rgba(cr, 1, 1, 1, 1); /\* White text for selected *\/ */
/*   } else { */
/*     cairo_set_source_rgba(cr, style->text_color[0], style->text_color[1], */
/*                           style->text_color[2], style->text_color[3]); */
/*   } */

/*   cairo_move_to(cr, style->padding, */
/*                 y_position + style->padding + style->font_size); */
/*   cairo_show_text(cr, item->label); */
/* } */

void cairo_menu_render_items(CairoMenuData *data, const Menu *menu) {
  // printf("Rendering items\n");
  // printf("Rendering items: data=%p, menu=%p\n", data, menu);
  const MenuStyle *style = &menu->config.style;
  double y = style->padding * 2 + style->font_size;
  for (size_t i = 0; i < menu->config.item_count; i++) {
    // printf("Rendering item %zu: %s\n", i, menu->config.items[i].label);
    cairo_menu_render_item(data, &menu->config.items[i], style,
                           (int)i == menu->selected_index, y);
    y += style->item_height;
  }
}

/* Size calculation */
void cairo_menu_render_calculate_size(CairoMenuData *data, const Menu *menu,
                                      int *width, int *height) {
  cairo_t *cr = data->render.cr;
  const MenuStyle *style = &menu->config.style;

  /* Set up font */
  cairo_select_font_face(cr, style->font_face, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, style->font_size);

  /* Calculate required width */
  double max_width = 0;
  cairo_text_extents_t extents;

  /* Check title width */
  cairo_text_extents(cr, menu->config.title, &extents);
  max_width = extents.width;

  /* Check menu items */
  for (size_t i = 0; i < menu->config.item_count; i++) {
    cairo_text_extents(cr, menu->config.items[i].label, &extents);
    if (extents.width > max_width) {
      max_width = extents.width;
    }
  }

  /* Calculate dimensions */
  *width = (int)(max_width + style->padding * 2);
  *height =
      (int)(menu->config.item_count * style->item_height + style->padding * 2);
}

/* Font handling */
void cairo_menu_render_set_font(CairoMenuData *data, const char *face,
                                double size, bool bold) {
  cairo_select_font_face(data->render.cr, face, CAIRO_FONT_SLANT_NORMAL,
                         bold ? CAIRO_FONT_WEIGHT_BOLD
                              : CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(data->render.cr, size);
}

/* Transform operations */
void cairo_menu_render_save_state(CairoMenuData *data) {
  cairo_save(data->render.cr);
}

void cairo_menu_render_restore_state(CairoMenuData *data) {
  cairo_restore(data->render.cr);
}

void cairo_menu_render_translate(CairoMenuData *data, double x, double y) {
  cairo_translate(data->render.cr, x, y);
}

void cairo_menu_render_scale(CairoMenuData *data, double sx, double sy) {
  cairo_scale(data->render.cr, sx, sy);
}

/*  */
void cairo_menu_render_set_opacity(CairoMenuData *data, double opacity) {
  cairo_push_group(data->render.cr);
  cairo_pop_group_to_source(data->render.cr);
  cairo_paint_with_alpha(data->render.cr, opacity);
}

/* Color operations */
void cairo_menu_render_set_color(CairoMenuData *data, const double color[4]) {
  cairo_set_source_rgba(data->render.cr, color[0], color[1], color[2],
                        color[3]);
}

/* State management */
bool cairo_menu_render_needs_update(const CairoMenuData *data) {
  return data->render.needs_redraw;
}

void cairo_menu_render_request_update(CairoMenuData *data) {
  data->render.needs_redraw = true;
}

/* Utility functions */
cairo_t *cairo_menu_render_get_context(CairoMenuData *data) {
  return data->render.cr;
}

xcb_window_t cairo_menu_render_get_window(CairoMenuData *data) {
  return data->render.window;
}

void cairo_menu_render_get_size(const CairoMenuData *data, int *width,
                                int *height) {
  if (width)
    *width = data->render.width;
  if (height)
    *height = data->render.height;
}

void cairo_menu_render_get_text_extents(CairoMenuData *data, const char *text,
                                        double *width, double *height) {
  cairo_text_extents_t extents;
  cairo_text_extents(data->render.cr, text, &extents);

  if (width)
    *width = extents.width;
  if (height)
    *height = extents.height;
}
