/* cairo_menu.c - Cairo-based menu rendering */
#include "cairo_menu.h"
#include "cairo_menu_animation.h"
#include "cairo_menu_render.h"
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[CAIRO_MENU]"
#include "log.h"
#endif
/* Forward declarations */
static void cairo_menu_cleanup(void *user_data);
static void cairo_menu_update(Menu *menu, void *user_data);
static bool cairo_menu_handle_expose(Menu *menu, void *user_data);
static xcb_visualtype_t *get_root_visual_type(xcb_screen_t *screen);

/* Create Cairo-rendered menu */
Menu *cairo_menu_create(xcb_connection_t *conn, xcb_window_t parent,
                        xcb_screen_t *screen, const MenuConfig *config) {
  LOG("Creating menu %s", config->title);
  if (!conn || !config) {
    fprintf(stderr, "Invalid connection or configuration\n");
    fprintf(stderr, "Connection: %p, Config: %p\n", conn, config);
    return NULL;
  }

  /* Create menu instance */
  Menu *menu = menu_create((MenuConfig *)config);
  if (!menu) {
    fprintf(stderr, "Failed to create menu\n");
    return NULL;
  }
  LOG("Menu created successfully\n");

  /* Create Cairo data */
  CairoMenuData *data = calloc(1, sizeof(CairoMenuData));
  if (!data) {
    fprintf(stderr, "Failed to allocate CairoMenuData\n");
    menu_destroy(menu);
    return NULL;
  }
  LOG("CairoMenuData allocated successfully\n");

  /* Get visual type for Cairo */
  xcb_visualtype_t *visual = get_root_visual_type(screen);
  if (!visual) {
    fprintf(stderr, "Failed to get visual type\n");
    free(data);
    menu_destroy(menu);
    return NULL;
  }
  LOG("Visual type retrieved successfully\n");

  /* Initialize rendering */
  if (!cairo_menu_render_init(data, conn, parent, visual)) {
    fprintf(stderr, "Failed to initialize rendering\n");
    free(data);
    menu_destroy(menu);
    return NULL;
  }
  data->conn = conn;
  /* data->render.window = parent; */
  data->menu = menu;
  LOG("Rendering initialized successfully\n");

  /* Initialize animations */
  /* cairo_menu_animation_init(data); */
  /* LOG("Animations initialized successfully\n"); */
  /* cairo_menu_animation_set_default(data, MENU_ANIM_FADE, MENU_ANIM_FADE,
   * 200.0); */

  /* cairo_menu_animation_set_sequence(data, true, NULL); */
  /* data->anim.show_animation = menu_animation_fade_in(200); /\* 200ms fade in
   */
  /*                                                           *\/ */
  /* cairo_menu_animation_show(data, menu); */
  /* double duration = 200.0; */

  /* while (duration > 0.0) { */
  /*   cairo_menu_animation_update(data, menu, duration); */
  /*   /\* cairo_menu_animation_show(data, menu); *\/ */
  /*   duration -= 10.0; */
  /* } */

  /* Set up menu callbacks */
  menu->cleanup_cb = cairo_menu_cleanup;
  menu->update_cb = cairo_menu_update;
  menu->user_data = data;
  // retrieve render like
  // cairo_surface_t *surface = ((CairoMenuData
  // *)menu->user_data)->render.surface; or window: xcb_window_t window =
  // ((CairoMenuData *)menu->user_data)->render.window;
  /* data->menu = menu; */
  /* data->anim.last_frame.tv_sec = 0; */

  return menu;
}

/* Cleanup resources */
static void cairo_menu_cleanup(void *user_data) {
  CairoMenuData *data = user_data;
  if (!data)
    return;

  cairo_menu_animation_cleanup(data);
  cairo_menu_render_cleanup(data);
  free(data);
}

/* Update menu state */
static void cairo_menu_update(Menu *menu, void *user_data) {
  CairoMenuData *data = user_data;
  if (!data)
    return;

  /* Handle animation updates */
  struct timeval now;
  gettimeofday(&now, NULL);

  double delta = (now.tv_sec - data->anim.last_frame.tv_sec) * 1000.0 +
                 (now.tv_usec - data->anim.last_frame.tv_usec) / 1000.0;

  cairo_menu_animation_update(data, menu, delta);
  data->anim.last_frame = now;

  if (cairo_menu_render_needs_update(data)) {
    cairo_menu_render_begin(data);
    cairo_menu_render_clear(data, &menu->config.style);
    cairo_menu_render_title(data, menu->config.title, &menu->config.style);
    cairo_menu_render_items(data, menu);
    cairo_menu_render_end(data);
  }
}

/* Handle expose event */
static bool cairo_menu_handle_expose(Menu *menu, void *user_data) {
  CairoMenuData *data = user_data;
  if (!data)
    return false;

  cairo_menu_render_request_update(data);
  return true;
}

/* Get visual type for Cairo */
static xcb_visualtype_t *get_root_visual_type(xcb_screen_t *screen) {
  /* xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
   */
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter =
        xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        return visual_iter.data;
      }
    }
  }
  return NULL;
}

/* Animation configuration */
void cairo_menu_set_animation(Menu *menu, MenuAnimationType show_anim,
                              MenuAnimationType hide_anim, double duration) {
  if (!menu || !menu->user_data)
    return;
  CairoMenuData *data = menu->user_data;

  cairo_menu_animation_set_default(data, show_anim, hide_anim, duration);
}

/* Custom animation sequences */
void cairo_menu_set_show_sequence(Menu *menu, MenuAnimationSequence *sequence) {
  if (!menu || !menu->user_data)
    return;
  CairoMenuData *data = menu->user_data;

  cairo_menu_animation_set_sequence(data, true, sequence);
}

void cairo_menu_set_hide_sequence(Menu *menu, MenuAnimationSequence *sequence) {
  if (!menu || !menu->user_data)
    return;
  CairoMenuData *data = menu->user_data;

  cairo_menu_animation_set_sequence(data, false, sequence);
}

/* Log menu activation */
void cairo_menu_activate(Menu *menu) {
  if (!menu || !menu->user_data)
    return;
  CairoMenuData *data = menu->user_data;

  cairo_menu_animation_show(data, menu);

  LOG("Menu activated.\n");
}

/* Log menu deactivation */
void cairo_menu_deactivate(Menu *menu) {
  if (!menu || !menu->user_data)
    return;
  CairoMenuData *data = menu->user_data;

  cairo_menu_animation_hide(data, menu);
  LOG("Menu deactivated.\n");
}

Menu *cairo_menu_init(const MenuConfig *config) {
  if (!config) {
    return NULL;
  }

  Menu *menu = calloc(1, sizeof(Menu));
  if (!menu) {
    return NULL;
  }

  menu->config = *config;

  return menu;
}

void cairo_menu_show(Menu *menu) {
  if (!menu || menu->active)
    return;
  menu->active = true;
  /* menu->state = MENU_STATE_ACTIVATING; */
  cairo_menu_update(menu, menu->user_data);

  /* double duration = 200.0; */
  /* for (size_t i = 0; i < 1000; i++) { */
  /*   if (menu->update_interval > 0 && menu->update_cb) */
  /*     menu_trigger_update(menu); */
  /* } */
  /* /\* while (duration > 0.0) { *\/ */
  /* cairo_menu_animation_update(NULL, menu, duration); */
  /*   duration -= 10.0; */
  /* } */
  /* while (menu->state == MENU_STATE_INITIALIZING) { */
  /* cairo_menu_update(menu, menu->user_data); */
  /* } */
}
