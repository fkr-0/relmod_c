
#ifndef MENU_DEFAULTS_H
#define MENU_DEFAULTS_H
/* menu_defaults.h */
#pragma once

#include "menu.h"

/* Default initializer for ActivationConfig */
static inline ActivationConfig activation_config_default() {
  return (ActivationConfig){.activate_on_mod_release = true,
                            .activate_on_direct_key = true,
                            .custom_activate = NULL,
                            .user_data = NULL};
}

/* Default initializer for NavigationConfig */
static inline NavigationConfig navigation_config_default() {
  return (NavigationConfig){.next = {.key = 0, .label = NULL},
                            .prev = {.key = 0, .label = NULL},
                            .direct = {.keys = NULL, .count = 0},
                            .extension_data = NULL};
}

/* Default initializer for MenuStyle */
static inline MenuStyle menu_style_default() {
  return (MenuStyle){.background_color = {0.0, 0.0, 0.0, 0.5}, // RGBA
                     .text_color = {0.2, 0.2, 1.0, 1.0},       // White
                     .highlight_color = {0.2, 0.6, 1.0, 1.0},  // Blue
                     .font_face = "Sans",
                     .font_size = 14.0,
                     .item_height = 24,
                     .padding = 8};
}
static inline ActivationState activation_state_default() {
  return (ActivationState){.config = NULL,
                           .mod_key = 0,
                           .keycode = 0,
                           .initialized = false,
                           .menu = NULL};
}

/* Default initializer for MenuConfig */
static inline MenuConfig menu_config_default() {
  return (MenuConfig){.mod_key = 0,
                      .trigger_key = 0,
                      .title = NULL,
                      .items = NULL,
                      .item_count = 0,
                      .nav = navigation_config_default(),
                      .act = activation_config_default(),
                      .style = menu_style_default(),
                      .act_state = activation_state_default()};
}

/* Optional: Default initializer for MenuItem (not always needed) */
static inline MenuItem menu_item_default() {
  return (MenuItem){
      .id = NULL, .label = NULL, .action = NULL, .metadata = NULL};
}

#endif /* MENU_DEFAULTS_H */
