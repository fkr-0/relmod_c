/* cairo_menu.c - Cairo-based menu rendering */
#include "cairo_menu.h"
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h> // Add this include for xcb_aux_* functions

/* Cairo menu data */
typedef struct {
  xcb_window_t window;
  cairo_surface_t *surface;
  cairo_t *cr;
  int width;
  int height;
  bool needs_redraw;

  /* Animation state */
  MenuAnimation *show_animation;
  MenuAnimation *hide_animation;
  MenuAnimationSequence *show_sequence;
  MenuAnimationSequence *hide_sequence;
  struct timeval last_frame;
  bool is_animating;
} CairoMenuData;

/* Forward declarations */
static void cairo_menu_cleanup(void *user_data);
static void cairo_menu_update(Menu *menu, void *user_data);
static bool cairo_menu_handle_expose(Menu *menu, void *user_data);
static void calculate_menu_size(Menu *menu, CairoMenuData *data);
static void draw_menu(Menu *menu, CairoMenuData *data);
static bool cairo_menu_action(uint8_t keycode,
                              void *user_data); // Add this declaration

/* Implementation of public functions */

Menu *cairo_menu_create(xcb_connection_t *conn, xcb_window_t parent,
                        MenuConfig *config) {
  if (!conn || !config) {
    return NULL;
  }

  Menu *menu = menu_create(NULL, config);
  if (!menu) {
    return NULL;
  }

  CairoMenuData *data = calloc(1, sizeof(CairoMenuData));
  if (!data) {
    menu_destroy(menu);
    return NULL;
  }

  data->window = parent;
  data->needs_redraw = true;

  // Set up Cairo surface and context
  xcb_visualtype_t *visual = xcb_aux_find_visual_by_id(
      xcb_aux_get_screen(conn, 0), xcb_aux_get_depth(conn, 0));

  data->surface = cairo_xcb_surface_create(
      conn, parent, visual, config->style.item_height * config->item_count,
      config->style.item_height * config->item_count);

  if (cairo_surface_status(data->surface) != CAIRO_STATUS_SUCCESS) {
    free(data);
    menu_destroy(menu);
    return NULL;
  }

  data->cr = cairo_create(data->surface);
  if (cairo_status(data->cr) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy(data->surface);
    free(data);
    menu_destroy(menu);
    return NULL;
  }

  // Initialize menu configuration
  menu->conn = conn;
  menu->user_data = data;
  menu->action_cb = cairo_menu_action;
  menu->cleanup_cb = cairo_menu_cleanup;
  menu->update_cb = cairo_menu_update;
  menu->update_interval = 16; // ~60 FPS

  // Calculate initial size
  calculate_menu_size(menu, data);

  return menu;
}

void cairo_menu_set_animation(Menu *menu, MenuAnimationType show_anim,
                              MenuAnimationType hide_anim, double duration) {
  if (!menu || !menu->user_data) {
    return;
  }

  CairoMenuData *data = (CairoMenuData *)menu->user_data;

  // Clean up existing animations
  if (data->show_animation) {
    menu_animation_free(data->show_animation);
  }
  if (data->hide_animation) {
    menu_animation_free(data->hide_animation);
  }

  // Create new animations
  data->show_animation = menu_animation_create(show_anim, duration);
  data->hide_animation = menu_animation_create(hide_anim, duration);

  // Create default sequences if none exist
  if (!data->show_sequence) {
    data->show_sequence = menu_animation_sequence_create();
    menu_animation_sequence_add(data->show_sequence, data->show_animation);
  }
  if (!data->hide_sequence) {
    data->hide_sequence = menu_animation_sequence_create();
    menu_animation_sequence_add(data->hide_sequence, data->hide_animation);
  }

  data->is_animating = true;
}

void cairo_menu_set_show_sequence(Menu *menu, MenuAnimationSequence *sequence) {
  if (!menu || !menu->user_data) {
    return;
  }

  CairoMenuData *data = (CairoMenuData *)menu->user_data;

  if (data->show_sequence) {
    menu_animation_sequence_free(data->show_sequence);
  }

  data->show_sequence = sequence;
}

void cairo_menu_set_hide_sequence(Menu *menu, MenuAnimationSequence *sequence) {
  if (!menu || !menu->user_data) {
    return;
  }

  CairoMenuData *data = (CairoMenuData *)menu->user_data;

  if (data->hide_sequence) {
    menu_animation_sequence_free(data->hide_sequence);
  }

  data->hide_sequence = sequence;
}

/* Implementation of static functions */

static bool cairo_menu_action(uint8_t keycode, void *user_data) {
  (void)keycode; /* Explicitly mark parameter as unused */
  CairoMenuData *data = (CairoMenuData *)user_data;

  if (!data) {
    return false;
  }

  data->needs_redraw = true;
  return true;
}

static void cairo_menu_cleanup(void *user_data) {
  if (!user_data) {
    return;
  }

  CairoMenuData *data = (CairoMenuData *)user_data;

  if (data->cr) {
    cairo_destroy(data->cr);
  }

  if (data->surface) {
    cairo_surface_destroy(data->surface);
  }

  if (data->show_animation) {
    menu_animation_free(data->show_animation);
  }

  if (data->hide_animation) {
    menu_animation_free(data->hide_animation);
  }

  free(data);
}

static void cairo_menu_update(Menu *menu, void *user_data) {
  if (!menu || !user_data) {
    return;
  }

  CairoMenuData *data = (CairoMenuData *)user_data;

  if (data->needs_redraw) {
    draw_menu(menu, data);
    data->needs_redraw = false;
  }

  if (data->is_animating) {
    struct timeval now;
    gettimeofday(&now, NULL);

    // Calculate time delta
    long delta_us = (now.tv_sec - data->last_frame.tv_sec) * 1000000 +
                    (now.tv_usec - data->last_frame.tv_usec);
    float delta = delta_us / 1000000.0f;

    // Update animation state
    if (menu->active && data->show_sequence) {
      menu_animation_sequence_update(data->show_sequence, delta);
    } else if (!menu->active && data->hide_sequence) {
      menu_animation_sequence_update(data->hide_sequence, delta);
    }

    data->last_frame = now;
    data->needs_redraw = true;
  }
}

static bool cairo_menu_handle_expose(Menu *menu, void *user_data) {
  if (!menu || !user_data) {
    return false;
  }

  CairoMenuData *data = (CairoMenuData *)user_data;
  draw_menu(menu, data);
  return true;
}

static void calculate_menu_size(Menu *menu, CairoMenuData *data) {
  if (!menu || !data || !data->cr) {
    return;
  }

  cairo_text_extents_t extents;
  int max_width = 0;
  int total_height = 0;
  const int padding = 10;
  const int item_height = 25;

  // Calculate required size for all menu items
  for (int i = 0; i < menu->config.item_count; i++) {
    cairo_text_extents(data->cr, menu->config.items[i].label, &extents);
    int item_width = extents.width + (padding * 2);
    if (item_width > max_width) {
      max_width = item_width;
    }
    total_height += item_height;
  }

  data->width = max_width;
  data->height = total_height;
}

static void draw_menu(Menu *menu, CairoMenuData *data) {
  if (!menu || !data || !data->cr) {
    return;
  }

  // Clear background
  cairo_set_source_rgb(data->cr, 0.2, 0.2, 0.2);
  cairo_paint(data->cr);

  // Draw menu items
  const int item_height = 25;
  const int text_padding = 10;

  cairo_select_font_face(data->cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(data->cr, 14);

  for (int i = 0; i < menu->config.item_count; i++) {
    int y = i * item_height;

    // Highlight selected item
    if (i == menu->selected_index) {
      cairo_set_source_rgb(data->cr, 0.3, 0.3, 0.3);
      cairo_rectangle(data->cr, 0, y, data->width, item_height);
      cairo_fill(data->cr);
    }

    // Draw item text
    cairo_set_source_rgb(data->cr, 0.9, 0.9, 0.9);
    cairo_move_to(data->cr, text_padding, y + item_height - text_padding);
    cairo_show_text(data->cr, menu->config.items[i].label);
  }

  cairo_surface_flush(data->surface);
}
