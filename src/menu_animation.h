/* menu_animation.h - Animation support for menus */
#ifndef MENU_ANIMATION_H
#define MENU_ANIMATION_H

#include <stdbool.h>
#include <stdint.h>

/* Animation type identifiers */
typedef enum {
  MENU_ANIM_NONE = 0,
  MENU_ANIM_FADE,
  MENU_ANIM_SLIDE_RIGHT,
  MENU_ANIM_SLIDE_LEFT,
  MENU_ANIM_SLIDE_UP,
  MENU_ANIM_SLIDE_DOWN,
  MENU_ANIM_ZOOM
} MenuAnimationType;

/* Animation properties */
typedef struct {
  double duration;      /* Animation duration in milliseconds */
  double current_time;  /* Current animation time */
  double start_value;   /* Starting value */
  double end_value;     /* Target value */
  double current_value; /* Current interpolated value */
  bool is_running;      /* Animation state */
} MenuAnimationProperty;

/* Animation state tracking */
typedef struct {
  MenuAnimationType type;
  MenuAnimationProperty opacity;
  MenuAnimationProperty position_x;
  MenuAnimationProperty position_y;
  MenuAnimationProperty scale;
  void (*completion_callback)(void *user_data);
  void *user_data;
} MenuAnimation;

/* Animation timing functions */
typedef double (*MenuAnimationEasing)(double progress);

/* Standard easing functions */
double menu_anim_linear(double progress);
double menu_anim_ease_in(double progress);
double menu_anim_ease_out(double progress);
double menu_anim_ease_in_out(double progress);
double menu_anim_bounce(double progress);

/* Animation initialization */
MenuAnimation *menu_animation_create(MenuAnimationType type, double duration);
void menu_animation_destroy(MenuAnimation *anim);

/* Animation control */
void menu_animation_start(MenuAnimation *anim);
void menu_animation_stop(MenuAnimation *anim);
void menu_animation_reset(MenuAnimation *anim);
void menu_animation_update(MenuAnimation *anim, double delta_time);
void menu_animation_free(MenuAnimation *anim);

/* Animation property configuration */
void menu_animation_set_opacity(MenuAnimation *anim, double start, double end);
void menu_animation_set_position(MenuAnimation *anim, double start_x,
                                 double end_x, double start_y, double end_y);
void menu_animation_set_scale(MenuAnimation *anim, double start, double end);

/* Animation completion handling */
void menu_animation_set_completion(MenuAnimation *anim,
                                   void (*callback)(void *user_data),
                                   void *user_data);

/* Animation state querying */
bool menu_animation_is_running(MenuAnimation *anim);
double menu_animation_get_progress(MenuAnimation *anim);

/* Current animation values */
double menu_animation_get_opacity(MenuAnimation *anim);
void menu_animation_get_position(MenuAnimation *anim, double *x, double *y);
double menu_animation_get_scale(MenuAnimation *anim);

/* Predefined animations */
MenuAnimation *menu_animation_fade_in(double duration);
MenuAnimation *menu_animation_fade_out(double duration);
MenuAnimation *menu_animation_slide_in(MenuAnimationType direction,
                                       double duration);
MenuAnimation *menu_animation_slide_out(MenuAnimationType direction,
                                        double duration);
MenuAnimation *menu_animation_zoom_in(double duration);
MenuAnimation *menu_animation_zoom_out(double duration);

/* Animation chaining */
typedef struct MenuAnimationSequence MenuAnimationSequence;

MenuAnimationSequence *menu_animation_sequence_create(void);
void menu_animation_sequence_destroy(MenuAnimationSequence *seq);
void menu_animation_sequence_add(MenuAnimationSequence *seq,
                                 MenuAnimation *anim);
void menu_animation_sequence_start(MenuAnimationSequence *seq);
void menu_animation_sequence_stop(MenuAnimationSequence *seq);
bool menu_animation_sequence_is_running(MenuAnimationSequence *seq);
void menu_animation_sequence_update(MenuAnimationSequence *seq,
                                    double delta_time);
void menu_animation_sequence_free(MenuAnimationSequence *seq);

/* Animation utilities */
double menu_animation_interpolate(double start, double end, double progress,
                                  MenuAnimationEasing easing);

#endif /* MENU_ANIMATION_H */
