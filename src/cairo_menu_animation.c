/* cairo_menu_animation.c - Cairo menu animation implementation */
#include "cairo_menu_animation.h"
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>

/* Initialize animation data */
void cairo_menu_animation_init(CairoMenuData* data) {
    printf("Initializing cairo_menu_animation\n");
    data->anim.show_animation = menu_animation_fade_in(200);  /* 200ms fade in */
    printf("Show animation initialized: %p\n", data->anim.show_animation);
    data->anim.hide_animation = menu_animation_fade_out(150); /* 150ms fade out */
    printf("Hide animation initialized: %p\n", data->anim.hide_animation);

    data->anim.show_sequence = NULL; // Initialize to NULL
    data->anim.hide_sequence = NULL; // Initialize to NULL

    if (data->anim.show_animation) {
        menu_animation_set_completion(data->anim.show_animation,
                                      cairo_menu_animation_on_show_complete, data);
    }
    if (data->anim.hide_animation) {
        menu_animation_set_completion(data->anim.hide_animation,
                                      cairo_menu_animation_on_hide_complete, data);
    }

    gettimeofday(&data->anim.last_frame, NULL);
    data->anim.is_animating = false;
    printf("Finished initializing cairo_menu_animation\n");
}

/* Cleanup animation data */
void cairo_menu_animation_cleanup(CairoMenuData* data) {
    printf("Cleaning up show animation: %p\n", data->anim.show_animation);
    menu_animation_destroy(data->anim.show_animation);
    printf("Cleaning up hide animation: %p\n", data->anim.hide_animation);
    menu_animation_destroy(data->anim.hide_animation);
    printf("Cleaning up show sequence: %p\n", data->anim.show_sequence);
    menu_animation_sequence_destroy(data->anim.show_sequence);
    printf("Cleaning up hide sequence: %p\n", data->anim.hide_sequence);
    menu_animation_sequence_destroy(data->anim.hide_sequence);
    printf("Finished cleaning up animations\n");
}

/* Update animation state */
void cairo_menu_animation_update(CairoMenuData* data, Menu* menu, double delta_time) {
    if (data->anim.is_animating) {
        if (menu->state == MENU_STATE_INITIALIZING) {
            if (data->anim.show_sequence) {
                menu_animation_sequence_update(data->anim.show_sequence, delta_time);
            } else if (data->anim.show_animation) {
                menu_animation_update(data->anim.show_animation, delta_time);
            }
        } else if (menu->state == MENU_STATE_INACTIVE) {
            if (data->anim.hide_sequence) {
                menu_animation_sequence_update(data->anim.hide_sequence, delta_time);
            } else if (data->anim.hide_animation) {
                menu_animation_update(data->anim.hide_animation, delta_time);
            }
        }
    }
}

/* Apply animation transforms */
void cairo_menu_animation_apply(CairoMenuData* data, Menu* menu, cairo_t* cr) {
    MenuAnimation* anim = NULL;

    if (data->anim.is_animating) {
        if (menu->state == MENU_STATE_INITIALIZING) {
            anim = data->anim.show_animation;
        } else if (menu->state == MENU_STATE_INACTIVE) {
            anim = data->anim.hide_animation;
        }
    }

    if (anim) {
        /* Apply transforms */
        double x, y;
        menu_animation_get_position(anim, &x, &y);
        cairo_translate(cr, x, y);

        double scale = menu_animation_get_scale(anim);
        cairo_scale(cr, scale, scale);

        double opacity = menu_animation_get_opacity(anim);
        cairo_push_group(cr);
        cairo_pop_group_to_source(cr);
        cairo_paint_with_alpha(cr, opacity);
    }
}

/* Trigger show animation */
void cairo_menu_animation_show(CairoMenuData* data, Menu* menu) {
    data->anim.is_animating = true;
    menu->state = MENU_STATE_INITIALIZING;
    menu_animation_start(data->anim.show_animation);
}

/* Trigger hide animation */
void cairo_menu_animation_hide(CairoMenuData* data, Menu* menu) {
    data->anim.is_animating = true;
    menu->state = MENU_STATE_INACTIVE;
    menu_animation_start(data->anim.hide_animation);
}

/* Set default animations */
void cairo_menu_animation_set_default(CairoMenuData* data,
                                      MenuAnimationType show_type,
                                      MenuAnimationType hide_type,
                                      double duration) {
    /* Clean up existing animations */
    menu_animation_destroy(data->anim.show_animation);
    menu_animation_destroy(data->anim.hide_animation);

    /* Create new animations */
    switch (show_type) {
        case MENU_ANIM_FADE:
            data->anim.show_animation = menu_animation_fade_in(duration);
            break;
        case MENU_ANIM_SLIDE_RIGHT:
        case MENU_ANIM_SLIDE_LEFT:
        case MENU_ANIM_SLIDE_UP:
        case MENU_ANIM_SLIDE_DOWN:
            data->anim.show_animation = menu_animation_slide_in(show_type, duration);
            break;
        case MENU_ANIM_ZOOM:
            data->anim.show_animation = menu_animation_zoom_in(duration);
            break;
        default:
            data->anim.show_animation = NULL;
    }

    switch (hide_type) {
        case MENU_ANIM_FADE:
            data->anim.hide_animation = menu_animation_fade_out(duration);
            break;
        case MENU_ANIM_SLIDE_RIGHT:
        case MENU_ANIM_SLIDE_LEFT:
        case MENU_ANIM_SLIDE_UP:
        case MENU_ANIM_SLIDE_DOWN:
            data->anim.hide_animation = menu_animation_slide_out(hide_type, duration);
            break;
        case MENU_ANIM_ZOOM:
            data->anim.hide_animation = menu_animation_zoom_out(duration);
            break;
        default:
            data->anim.hide_animation = NULL;
    }

    /* Set completion callbacks */
    if (data->anim.show_animation) {
        menu_animation_set_completion(data->anim.show_animation,
                                      cairo_menu_animation_on_show_complete, data);
    }
    if (data->anim.hide_animation) {
        menu_animation_set_completion(data->anim.hide_animation,
                                      cairo_menu_animation_on_hide_complete, data);
    }
}

/* Set custom animation sequences */
void cairo_menu_animation_set_sequence(CairoMenuData* data,
                                       bool is_show,
                                       MenuAnimationSequence* sequence) {
    if (is_show) {
        menu_animation_sequence_destroy(data->anim.show_sequence);
        data->anim.show_sequence = sequence;
    } else {
        menu_animation_sequence_destroy(data->anim.hide_sequence);
        data->anim.hide_sequence = sequence;
    }
}

/* Animation completion callbacks */
void cairo_menu_animation_on_show_complete(void* user_data) {
    CairoMenuData* data = user_data;
    data->anim.is_animating = false;
    data->menu->state = MENU_STATE_ACTIVE;
}

void cairo_menu_animation_on_hide_complete(void* user_data) {
    CairoMenuData* data = user_data;
    data->anim.is_animating = false;
    data->menu->state = MENU_STATE_INACTIVE;
    xcb_unmap_window(data->conn, data->render.window);
}

/* Animation state queries */
bool cairo_menu_animation_is_active(CairoMenuData* data) {
    return data->anim.is_animating;
}

double cairo_menu_animation_get_progress(CairoMenuData* data) {
    if (data->anim.show_animation) {
        return menu_animation_get_progress(data->anim.show_animation);
    }
    return 0.0;
}
