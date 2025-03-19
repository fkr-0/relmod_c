/* cairo_menu_animation.h - Cairo menu animation support */
#ifndef CAIRO_MENU_ANIMATION_H
#define CAIRO_MENU_ANIMATION_H

#include "menu.h"
#include "menu_animation.h"
#include <cairo/cairo.h>
#include <stdbool.h>
#include <sys/time.h>

/* Menu animation data */
typedef struct CairoMenuAnimData {
    MenuAnimation* show_animation;
    MenuAnimation* hide_animation;
    MenuAnimationSequence* show_sequence;
    MenuAnimationSequence* hide_sequence;
    struct timeval last_frame;
    bool is_animating;
} CairoMenuAnimData;

/* Forward declaration */
typedef struct CairoMenuData CairoMenuData;

/* Animation management */
void cairo_menu_animation_init(CairoMenuData* data);
void cairo_menu_animation_cleanup(CairoMenuData* data);

/* Animation updates */
void cairo_menu_animation_update(CairoMenuData* data, Menu* menu, double delta_time);
void cairo_menu_animation_apply(CairoMenuData* data, Menu* menu, cairo_t* cr);

/* Animation triggers */
void cairo_menu_animation_show(CairoMenuData* data, Menu* menu);
void cairo_menu_animation_hide(CairoMenuData* data, Menu* menu);

/* Animation configuration */
void cairo_menu_animation_set_default(CairoMenuData* data,
                                    MenuAnimationType show_type,
                                    MenuAnimationType hide_type,
                                    double duration);

void cairo_menu_animation_set_sequence(CairoMenuData* data,
                                     bool is_show,
                                     MenuAnimationSequence* sequence);

/* Animation completion callbacks */
void cairo_menu_animation_on_show_complete(void* user_data);
void cairo_menu_animation_on_hide_complete(void* user_data);

/* Animation state queries */
bool cairo_menu_animation_is_active(CairoMenuData* data);
double cairo_menu_animation_get_progress(CairoMenuData* data);

#endif /* CAIRO_MENU_ANIMATION_H */