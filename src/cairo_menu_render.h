/* cairo_menu_render.h - Cairo menu rendering support */
#ifndef CAIRO_MENU_RENDER_H
#define CAIRO_MENU_RENDER_H

#include "menu.h"
#include "menu_animation.h"
#include "x11_focus.h"
#include <cairo/cairo.h>
#include <stdbool.h>
#include <xcb/xcb.h>

/* Menu rendering data */
typedef struct CairoMenuRenderData {
  xcb_window_t window;      /* X11 window */
  cairo_surface_t *surface; /* Cairo surface */
  cairo_t *cr;              /* Cairo context */
  int width;                /* Window width */
  int height;               /* Window height */
  bool needs_redraw;        /* Redraw flag */
} CairoMenuRenderData;

/* Menu animation data */
typedef struct CairoMenuAnimData {
  MenuAnimation *show_animation;
  MenuAnimation *hide_animation;
  MenuAnimationSequence *show_sequence;
  MenuAnimationSequence *hide_sequence;
  struct timeval last_frame;
  bool is_animating;
} CairoMenuAnimData;

/* Combined menu data */
typedef struct CairoMenuData {
  xcb_connection_t *conn;     /* X11 connection */
  CairoMenuRenderData render; /* Rendering data */
  CairoMenuAnimData anim;     /* Animation data */
  Menu *menu;                 /* Menu reference */
} CairoMenuData;

/* Rendering initialization */
bool cairo_menu_render_init(CairoMenuData *data, xcb_connection_t *conn,
                            xcb_screen_t *screen, xcb_window_t parent,
                            X11FocusContext *ctx, xcb_visualtype_t *visual);

/* Window management */
void cairo_menu_render_cleanup(CairoMenuData *data);
void cairo_menu_render_show(CairoMenuData *data);
void cairo_menu_render_hide(CairoMenuData *data);
void cairo_menu_render_resize(CairoMenuData *data, int width, int height);

/* Rendering operations */
void cairo_menu_render_begin(CairoMenuData *data);
void cairo_menu_render_end(CairoMenuData *data);
void cairo_menu_render_clear(CairoMenuData *data, const MenuStyle *style);

/* Menu content rendering */
void cairo_menu_render_title(CairoMenuData *data, const char *title,
                             const MenuStyle *style);

void cairo_menu_render_item(CairoMenuData *data, const MenuItem *item,
                            const MenuStyle *style, bool is_selected,
                            double y_position);

void cairo_menu_render_items(CairoMenuData *data, const Menu *menu);

/* Size calculation */
void cairo_menu_render_calculate_size(CairoMenuData *data, const Menu *menu,
                                      int *width, int *height);

/* Font handling */
void cairo_menu_render_set_font(CairoMenuData *data, const char *face,
                                double size, bool bold);

/* State management */
bool cairo_menu_render_needs_update(const CairoMenuData *data);
void cairo_menu_render_request_update(CairoMenuData *data);

/* Transform operations */
void cairo_menu_render_save_state(CairoMenuData *data);
void cairo_menu_render_restore_state(CairoMenuData *data);
void cairo_menu_render_translate(CairoMenuData *data, double x, double y);
void cairo_menu_render_scale(CairoMenuData *data, double sx, double sy);
void cairo_menu_render_set_opacity(CairoMenuData *data, double opacity);

/* Color operations */
void cairo_menu_render_set_color(CairoMenuData *data, const double color[4]);

/* Utility functions */
cairo_t *cairo_menu_render_get_context(CairoMenuData *data);
xcb_window_t cairo_menu_render_get_window(CairoMenuData *data);
void cairo_menu_render_get_size(const CairoMenuData *data, int *width,
                                int *height);

/* Text measurement */
void cairo_menu_render_get_text_extents(CairoMenuData *data, const char *text,
                                        double *width, double *height);

#endif /* CAIRO_MENU_RENDER_H */
