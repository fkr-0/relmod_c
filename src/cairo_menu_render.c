/* cairo_menu_render.c - Cairo menu rendering implementation */
#include "cairo_menu_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo/cairo-xcb.h>

/* Create menu window */
static xcb_window_t create_window(xcb_connection_t* conn,
                                xcb_window_t parent,
                                int width,
                                int height) {
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    
    uint32_t values[3];
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    
    values[0] = screen->black_pixel;
    values[1] = 1;  /* Override redirect */
    values[2] = XCB_EVENT_MASK_EXPOSURE;
    
    xcb_window_t window = xcb_generate_id(conn);
    xcb_create_window(conn,
                     XCB_COPY_FROM_PARENT,
                     window,
                     parent,
                     0, 0,            /* x, y */
                     width, height,    /* width, height */
                     0,               /* border width */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT,
                     screen->root_visual,
                     mask,
                     values);
    
    return window;
}

/* Initialize rendering */
bool cairo_menu_render_init(CairoMenuData* data,
                          xcb_connection_t* conn,
                          xcb_window_t parent,
                          xcb_visualtype_t* visual) {
    CairoMenuRenderData* render = &data->render;
    
    /* Create initial window */
    render->width = 200;   /* Default size */
    render->height = 300;
    render->window = create_window(conn, parent, render->width, render->height);
    
    if (render->window == XCB_NONE) {
        return false;
    }
    
    /* Create Cairo surface */
    render->surface = cairo_xcb_surface_create(conn,
                                             render->window,
                                             visual,
                                             render->width,
                                             render->height);
    
    if (cairo_surface_status(render->surface) != CAIRO_STATUS_SUCCESS) {
        xcb_destroy_window(conn, render->window);
        return false;
    }
    
    /* Create Cairo context */
    render->cr = cairo_create(render->surface);
    if (cairo_status(render->cr) != CAIRO_STATUS_SUCCESS) {
        cairo_surface_destroy(render->surface);
        xcb_destroy_window(conn, render->window);
        return false;
    }
    
    render->needs_redraw = true;
    return true;
}

/* Cleanup rendering resources */
void cairo_menu_render_cleanup(CairoMenuData* data) {
    CairoMenuRenderData* render = &data->render;
    
    if (render->cr) {
        cairo_destroy(render->cr);
        render->cr = NULL;
    }
    
    if (render->surface) {
        cairo_surface_destroy(render->surface);
        render->surface = NULL;
    }
    
    if (render->window != XCB_NONE) {
        xcb_destroy_window(data->conn, render->window);
        render->window = XCB_NONE;
    }
}

/* Window management */
void cairo_menu_render_show(CairoMenuData* data) {
    xcb_map_window(data->conn, data->render.window);
    xcb_flush(data->conn);
}

void cairo_menu_render_hide(CairoMenuData* data) {
    xcb_unmap_window(data->conn, data->render.window);
    xcb_flush(data->conn);
}

void cairo_menu_render_resize(CairoMenuData* data, int width, int height) {
    CairoMenuRenderData* render = &data->render;
    
    if (width == render->width && height == render->height) {
        return;
    }
    
    render->width = width;
    render->height = height;
    
    /* Update window size */
    uint32_t values[2] = { (uint32_t)width, (uint32_t)height };
    xcb_configure_window(data->conn,
                        render->window,
                        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                        values);
    
    /* Update surface size */
    cairo_xcb_surface_set_size(render->surface, width, height);
    render->needs_redraw = true;
}

/* Rendering operations */
void cairo_menu_render_begin(CairoMenuData* data) {
    cairo_save(data->render.cr);
}

void cairo_menu_render_end(CairoMenuData* data) {
    cairo_restore(data->render.cr);
    cairo_surface_flush(data->render.surface);
    xcb_flush(data->conn);
}

void cairo_menu_render_clear(CairoMenuData* data, const MenuStyle* style) {
    cairo_t* cr = data->render.cr;
    
    cairo_set_source_rgba(cr,
                         style->background_color[0],
                         style->background_color[1],
                         style->background_color[2],
                         style->background_color[3]);
    cairo_paint(cr);
}

/* Menu content rendering */
void cairo_menu_render_title(CairoMenuData* data,
                           const char* title,
                           const MenuStyle* style) {
    cairo_t* cr = data->render.cr;
    
    cairo_set_source_rgba(cr,
                         style->text_color[0],
                         style->text_color[1],
                         style->text_color[2],
                         style->text_color[3]);
    
    cairo_move_to(cr, style->padding, style->padding + style->font_size);
    cairo_show_text(cr, title);
}

void cairo_menu_render_item(CairoMenuData* data,
                          const MenuItem* item,
                          const MenuStyle* style,
                          bool is_selected,
                          double y_position) {
    cairo_t* cr = data->render.cr;
    
    /* Draw selection highlight */
    if (is_selected) {
        cairo_set_source_rgba(cr,
                            style->highlight_color[0],
                            style->highlight_color[1],
                            style->highlight_color[2],
                            style->highlight_color[3]);
        cairo_rectangle(cr, 0, y_position,
                       data->render.width, style->item_height);
        cairo_fill(cr);
        cairo_set_source_rgba(cr, 1, 1, 1, 1);  /* White text for selected */
    } else {
        cairo_set_source_rgba(cr,
                            style->text_color[0],
                            style->text_color[1],
                            style->text_color[2],
                            style->text_color[3]);
    }
    
    cairo_move_to(cr, style->padding,
                  y_position + style->padding + style->font_size);
    cairo_show_text(cr, item->label);
}

void cairo_menu_render_items(CairoMenuData* data,
                           const Menu* menu) {
    const MenuStyle* style = &menu->config.style;
    double y = style->padding * 2 + style->font_size;
    
    for (size_t i = 0; i < menu->config.item_count; i++) {
        cairo_menu_render_item(data,
                             &menu->config.items[i],
                             style,
                             (int)i == menu->selected_index,
                             y);
        y += style->item_height;
    }
}

/* Size calculation */
void cairo_menu_render_calculate_size(CairoMenuData* data,
                                   const Menu* menu,
                                   int* width,
                                   int* height) {
    cairo_t* cr = data->render.cr;
    const MenuStyle* style = &menu->config.style;
    
    /* Set up font */
    cairo_select_font_face(cr, style->font_face,
                          CAIRO_FONT_SLANT_NORMAL,
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
    *height = (int)(menu->config.item_count * style->item_height +
                   style->padding * 2);
}

/* Font handling */
void cairo_menu_render_set_font(CairoMenuData* data,
                              const char* face,
                              double size,
                              bool bold) {
    cairo_select_font_face(data->render.cr,
                          face,
                          CAIRO_FONT_SLANT_NORMAL,
                          bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(data->render.cr, size);
}

/* Transform operations */
void cairo_menu_render_save_state(CairoMenuData* data) {
    cairo_save(data->render.cr);
}

void cairo_menu_render_restore_state(CairoMenuData* data) {
    cairo_restore(data->render.cr);
}

void cairo_menu_render_translate(CairoMenuData* data, double x, double y) {
    cairo_translate(data->render.cr, x, y);
}

void cairo_menu_render_scale(CairoMenuData* data, double sx, double sy) {
    cairo_scale(data->render.cr, sx, sy);
}

void cairo_menu_render_set_opacity(CairoMenuData* data, double opacity) {
    cairo_push_group(data->render.cr);
    cairo_pop_group_to_source(data->render.cr);
    cairo_paint_with_alpha(data->render.cr, opacity);
}

/* Color operations */
void cairo_menu_render_set_color(CairoMenuData* data,
                               const double color[4]) {
    cairo_set_source_rgba(data->render.cr,
                         color[0], color[1], color[2], color[3]);
}

/* State management */
bool cairo_menu_render_needs_update(const CairoMenuData* data) {
    return data->render.needs_redraw;
}

void cairo_menu_render_request_update(CairoMenuData* data) {
    data->render.needs_redraw = true;
}

/* Utility functions */
cairo_t* cairo_menu_render_get_context(CairoMenuData* data) {
    return data->render.cr;
}

xcb_window_t cairo_menu_render_get_window(CairoMenuData* data) {
    return data->render.window;
}

void cairo_menu_render_get_size(const CairoMenuData* data,
                              int* width,
                              int* height) {
    if (width) *width = data->render.width;
    if (height) *height = data->render.height;
}

void cairo_menu_render_get_text_extents(CairoMenuData* data,
                                      const char* text,
                                      double* width,
                                      double* height) {
    cairo_text_extents_t extents;
    cairo_text_extents(data->render.cr, text, &extents);
    
    if (width) *width = extents.width;
    if (height) *height = extents.height;
}