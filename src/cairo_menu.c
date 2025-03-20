/* cairo_menu.c - Cairo-based menu rendering */
#include "cairo_menu.h"
#include "cairo_menu_animation.h"
#include "cairo_menu_render.h"
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Forward declarations */
static void cairo_menu_cleanup(void *user_data);
static void cairo_menu_update(Menu *menu, void *user_data);
static bool cairo_menu_handle_expose(Menu *menu, void *user_data);
static xcb_visualtype_t *get_root_visual_type(xcb_connection_t *conn);

/* Create Cairo-rendered menu */
Menu *cairo_menu_create(xcb_connection_t *conn, xcb_window_t parent, const MenuConfig *config) {
    if (!conn || !config) {
        fprintf(stderr, "Invalid connection or configuration\n");
        return NULL;
    }

    /* Create menu instance */
    Menu *menu = menu_create(NULL, (MenuConfig *)config);
    if (!menu) {
        fprintf(stderr, "Failed to create menu\n");
        return NULL;
    }

    /* Create Cairo data */
    CairoMenuData *data = calloc(1, sizeof(CairoMenuData));
    if (!data) {
        fprintf(stderr, "Failed to allocate CairoMenuData\n");
        menu_destroy(menu);
        return NULL;
    }

    /* Get visual type for Cairo */
    xcb_visualtype_t *visual = get_root_visual_type(conn);
    if (!visual) {
        fprintf(stderr, "Failed to get visual type\n");
        free(data);
        menu_destroy(menu);
        return NULL;
    }

    /* Initialize rendering */
    if (!cairo_menu_render_init(data, conn, parent, visual)) {
        fprintf(stderr, "Failed to initialize rendering\n");
        free(data);
        menu_destroy(menu);
        return NULL;
    }

    /* Initialize animations */
    cairo_menu_animation_init(data);

    /* Set up menu callbacks */
    menu->cleanup_cb = cairo_menu_cleanup;
    menu->update_cb = cairo_menu_update;
    menu->user_data = data;

    data->menu = menu;

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
static xcb_visualtype_t *get_root_visual_type(xcb_connection_t *conn) {
    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (screen->root_visual == visual_iter.data->visual_id) {
                return visual_iter.data;
            }
        }
    }
    return NULL;
}

/* Animation configuration */
void cairo_menu_set_animation(Menu *menu, MenuAnimationType show_anim, MenuAnimationType hide_anim, double duration) {
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
    printf("Menu activated.\n");
}

/* Log menu deactivation */
void cairo_menu_deactivate(Menu *menu) {
    if (!menu || !menu->user_data)
        return;
    CairoMenuData *data = menu->user_data;

    cairo_menu_animation_hide(data, menu);
    printf("Menu deactivated.\n");
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
