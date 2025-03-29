/* menu_animation.c - Animation implementation */
#include "menu_animation.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define SEQUENCE_INITIAL_CAPACITY 8

/* Animation sequence implementation */
struct MenuAnimationSequence {
    MenuAnimation **animations;
    size_t count;
    size_t capacity;
    size_t current;
    bool is_running;
};

/* Standard easing functions */
double menu_anim_linear(double progress) { return progress; }

double menu_anim_ease_in(double progress) { return progress * progress; }

double menu_anim_ease_out(double progress) {
    return 1.0 - (1.0 - progress) * (1.0 - progress);
}

double menu_anim_ease_in_out(double progress) {
    return progress < 0.5 ? 2.0 * progress * progress
                          : 1.0 - pow(-2.0 * progress + 2.0, 2) / 2.0;
}

double menu_anim_bounce(double progress) {
    if (progress < 4.0 / 11.0) {
        return (121.0 * progress * progress) / 16.0;
    } else if (progress < 8.0 / 11.0) {
        return (363.0 / 40.0 * progress * progress) - (99.0 / 10.0 * progress) +
               17.0 / 5.0;
    } else if (progress < 9.0 / 10.0) {
        return (4356.0 / 361.0 * progress * progress) -
               (35442.0 / 1805.0 * progress) + 16061.0 / 1805.0;
    } else {
        return (54.0 / 5.0 * progress * progress) - (513.0 / 25.0 * progress) +
               268.0 / 25.0;
    }
}

/* Animation creation and destruction */
MenuAnimation *menu_animation_create(MenuAnimationType type, double duration) {
    MenuAnimation *anim = calloc(1, sizeof(MenuAnimation));
    if (!anim)
        return NULL;

    anim->type = type;
    anim->opacity.duration = duration;
    anim->position_x.duration = duration;
    anim->position_y.duration = duration;
    anim->scale.duration = duration;

    return anim;
}

void menu_animation_destroy(MenuAnimation *anim) { free(anim); }

/* Animation control */
void menu_animation_start(MenuAnimation *anim) {
    if (!anim)
        return;

    anim->opacity.is_running = true;
    anim->position_x.is_running = true;
    anim->position_y.is_running = true;
    anim->scale.is_running = true;
    anim->opacity.current_time = 0;
    anim->position_x.current_time = 0;
    anim->position_y.current_time = 0;
    anim->scale.current_time = 0;
}

void menu_animation_stop(MenuAnimation *anim) {
    if (!anim)
        return;

    anim->opacity.is_running = false;
    anim->position_x.is_running = false;
    anim->position_y.is_running = false;
    anim->scale.is_running = false;
}

void menu_animation_reset(MenuAnimation *anim) {
    if (!anim)
        return;

    anim->opacity.current_time = 0;
    anim->position_x.current_time = 0;
    anim->position_y.current_time = 0;
    anim->scale.current_time = 0;

    anim->opacity.current_value = anim->opacity.start_value;
    anim->position_x.current_value = anim->position_x.start_value;
    anim->position_y.current_value = anim->position_y.start_value;
    anim->scale.current_value = anim->scale.start_value;
}

static void update_property(MenuAnimationProperty *prop, double delta_time) {
    if (!prop->is_running)
        return;

    prop->current_time += delta_time;
    if (prop->current_time >= prop->duration) {
        prop->current_time = prop->duration;
        prop->is_running = false;
        prop->current_value = prop->end_value;
    } else {
        double progress = prop->current_time / prop->duration;
        prop->current_value =
            menu_animation_interpolate(prop->start_value, prop->end_value,
                                       progress, menu_anim_ease_in_out);
    }
    // printf("Updated property: current_time=%f, duration=%f, current_value=%f,
    // end_value=%f\n",
    //        prop->current_time, prop->duration, prop->current_value,
    //        prop->end_value);
}

void menu_animation_update(MenuAnimation *anim, double delta_time) {
    if (!anim)
        return;
    // printf("Updating animation: anim=%p, delta_time=%f\n", anim, delta_time);
    update_property(&anim->opacity, delta_time);
    update_property(&anim->position_x, delta_time);
    update_property(&anim->position_y, delta_time);
    update_property(&anim->scale, delta_time);
    if (!anim->opacity.is_running && !anim->position_x.is_running &&
        !anim->position_y.is_running && !anim->scale.is_running) {
        if (anim->completion_callback) {
            anim->completion_callback(anim->user_data);
        }
    }
}

/* Property configuration */
void menu_animation_set_opacity(MenuAnimation *anim, double start, double end) {
    if (!anim)
        return;
    anim->opacity.start_value = start;
    anim->opacity.end_value = end;
    anim->opacity.current_value = start;
}

void menu_animation_set_position(MenuAnimation *anim, double start_x,
                                 double end_x, double start_y, double end_y) {
    if (!anim)
        return;
    anim->position_x.start_value = start_x;
    anim->position_x.end_value = end_x;
    anim->position_x.current_value = start_x;

    anim->position_y.start_value = start_y;
    anim->position_y.end_value = end_y;
    anim->position_y.current_value = start_y;
}

void menu_animation_set_scale(MenuAnimation *anim, double start, double end) {
    if (!anim)
        return;
    anim->scale.start_value = start;
    anim->scale.end_value = end;
    anim->scale.current_value = start;
}

/* Completion handling */
void menu_animation_set_completion(MenuAnimation *anim,
                                   void (*callback)(void *user_data),
                                   void *user_data) {
    if (!anim)
        return;
    anim->completion_callback = callback;
    anim->user_data = user_data;
}

/* State queries */
bool menu_animation_is_running(MenuAnimation *anim) {
    if (!anim)
        return false;
    return anim->opacity.is_running || anim->position_x.is_running ||
           anim->position_y.is_running || anim->scale.is_running;
}

double menu_animation_get_progress(MenuAnimation *anim) {
    if (!anim || anim->opacity.duration == 0)
        return 0.0;
    return anim->opacity.current_time / anim->opacity.duration;
}

/* Value getters */
double menu_animation_get_opacity(MenuAnimation *anim) {
    return anim ? anim->opacity.current_value : 1.0;
}

void menu_animation_get_position(MenuAnimation *anim, double *x, double *y) {
    if (!anim || !x || !y)
        return;
    *x = anim->position_x.current_value;
    *y = anim->position_y.current_value;
}

double menu_animation_get_scale(MenuAnimation *anim) {
    return anim ? anim->scale.current_value : 1.0;
}

/* Sequence management */
MenuAnimationSequence *menu_animation_sequence_create(void) {
    MenuAnimationSequence *seq = calloc(1, sizeof(MenuAnimationSequence));
    if (!seq)
        return NULL;

    seq->capacity = SEQUENCE_INITIAL_CAPACITY;
    seq->animations = calloc(seq->capacity, sizeof(MenuAnimation *));
    if (!seq->animations) {
        free(seq);
        return NULL;
    }

    return seq;
}

void menu_animation_sequence_destroy(MenuAnimationSequence *seq) {
    if (!seq)
        return;

    for (size_t i = 0; i < seq->count; i++) {
        menu_animation_destroy(seq->animations[i]);
    }
    free(seq->animations);
    free(seq);
}

void menu_animation_sequence_add(MenuAnimationSequence *seq,
                                 MenuAnimation *anim) {
    if (!seq || !anim)
        return;

    if (seq->count >= seq->capacity) {
        size_t new_capacity = seq->capacity * 2;
        MenuAnimation **new_anims =
            realloc(seq->animations, new_capacity * sizeof(MenuAnimation *));
        if (!new_anims)
            return;

        seq->animations = new_anims;
        seq->capacity = new_capacity;
    }

    seq->animations[seq->count++] = anim;
}

void menu_animation_sequence_start(MenuAnimationSequence *seq) {
    if (!seq || seq->count == 0)
        return;

    seq->current = 0;
    seq->is_running = true;
    menu_animation_start(seq->animations[0]);
}

void menu_animation_free(MenuAnimation *anim) {
    if (!anim)
        return;

    free(anim);
}

void menu_animation_sequence_stop(MenuAnimationSequence *seq) {
    if (!seq)
        return;

    if (seq->current < seq->count) {
        menu_animation_stop(seq->animations[seq->current]);
    }
    seq->is_running = false;
}

void menu_animation_sequence_free(MenuAnimationSequence *seq) {
    if (!seq)
        return;

    for (size_t i = 0; i < seq->count; i++) {
        menu_animation_free(seq->animations[i]);
    }
    free(seq->animations);
    free(seq);
}

bool menu_animation_sequence_is_running(MenuAnimationSequence *seq) {
    return seq ? seq->is_running : false;
}

void menu_animation_sequence_update(MenuAnimationSequence *seq,
                                    double delta_time) {
    if (!seq || !seq->is_running || seq->current >= seq->count)
        return;
    // printf("Updating animation sequence: seq=%p, delta_time=%f\n", seq,
    // delta_time);
    MenuAnimation *current = seq->animations[seq->current];
    menu_animation_update(current, delta_time);
    if (!menu_animation_is_running(current)) {
        seq->current++;
        if (seq->current < seq->count) {
            menu_animation_start(seq->animations[seq->current]);
        } else {
            seq->is_running = false;
        }
    }
}

/* Utility functions */
double menu_animation_interpolate(double start, double end, double progress,
                                  MenuAnimationEasing easing) {
    if (!easing)
        easing = menu_anim_linear;
    double eased = easing(progress);
    return start + (end - start) * eased;
}

/* Predefined animations */
MenuAnimation *menu_animation_fade_in(double duration) {
    MenuAnimation *anim = menu_animation_create(MENU_ANIM_FADE, duration);
    if (anim) {
        menu_animation_set_opacity(anim, 0.0, 1.0);
    }
    return anim;
}

MenuAnimation *menu_animation_fade_out(double duration) {
    MenuAnimation *anim = menu_animation_create(MENU_ANIM_FADE, duration);
    if (anim) {
        menu_animation_set_opacity(anim, 1.0, 0.0);
    }
    return anim;
}

MenuAnimation *menu_animation_slide_in(MenuAnimationType direction,
                                       double duration) {
    MenuAnimation *anim = menu_animation_create(direction, duration);
    if (!anim)
        return NULL;

    switch (direction) {
    case MENU_ANIM_SLIDE_RIGHT:
        menu_animation_set_position(anim, -100, 0, 0, 0);
        break;
    case MENU_ANIM_SLIDE_LEFT:
        menu_animation_set_position(anim, 100, 0, 0, 0);
        break;
    case MENU_ANIM_SLIDE_UP:
        menu_animation_set_position(anim, 0, 0, 100, 0);
        break;
    case MENU_ANIM_SLIDE_DOWN:
        menu_animation_set_position(anim, 0, 0, -100, 0);
        break;
    default:
        menu_animation_destroy(anim);
        return NULL;
    }

    return anim;
}

MenuAnimation *menu_animation_slide_out(MenuAnimationType direction,
                                        double duration) {
    MenuAnimation *anim = menu_animation_create(direction, duration);
    if (!anim)
        return NULL;

    switch (direction) {
    case MENU_ANIM_SLIDE_RIGHT:
        menu_animation_set_position(anim, 0, 100, 0, 0);
        break;
    case MENU_ANIM_SLIDE_LEFT:
        menu_animation_set_position(anim, 0, -100, 0, 0);
        break;
    case MENU_ANIM_SLIDE_UP:
        menu_animation_set_position(anim, 0, 0, 0, -100);
        break;
    case MENU_ANIM_SLIDE_DOWN:
        menu_animation_set_position(anim, 0, 0, 0, 100);
        break;
    default:
        menu_animation_destroy(anim);
        return NULL;
    }

    return anim;
}

MenuAnimation *menu_animation_zoom_in(double duration) {
    MenuAnimation *anim = menu_animation_create(MENU_ANIM_ZOOM, duration);
    if (anim) {
        menu_animation_set_scale(anim, 0.5, 1.0);
        menu_animation_set_opacity(anim, 0.0, 1.0);
    }
    return anim;
}

MenuAnimation *menu_animation_zoom_out(double duration) {
    MenuAnimation *anim = menu_animation_create(MENU_ANIM_ZOOM, duration);
    if (anim) {
        menu_animation_set_scale(anim, 1.0, 0.5);
        menu_animation_set_opacity(anim, 1.0, 0.0);
    }
    return anim;
}
