/* menu_builder.h - Declarative menu configuration and safe composition */
#ifndef MENU_BUILDER_H
#define MENU_BUILDER_H

#include "menu.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef ENABLE_MENU_TOML
#include <toml.h>
#endif

typedef struct MenuBuilder {
  MenuConfig config;
  MenuItem *items;
  size_t count;
  size_t capacity;
} MenuBuilder;

/* Create a menu builder for the specified title and item capacity */
MenuBuilder menu_builder_create(const char *title, size_t max_items);

/* Add a menu item to the builder */
bool menu_builder_add_item(MenuBuilder *builder, const char *label,
                           void (*action)(void *), void *metadata);
/* Struct setters for config */
void menu_builder_set_mod_key(MenuBuilder *builder, uint16_t mod_key);
void menu_builder_set_trigger_key(MenuBuilder *builder, uint8_t trigger_key);
void menu_builder_set_navigation_keys(MenuBuilder *builder, uint8_t next_key,
                                      const char *next_label, uint8_t prev_key,
                                      const char *prev_label,
                                      uint8_t *direct_keys,
                                      size_t direct_count);
void menu_builder_set_activation(MenuBuilder *builder, bool on_mod_release,
                                 bool on_direct_key);
void menu_builder_set_style(MenuBuilder *builder, double *background_color,
                            double *text_color, double *highlight_color,
                            const char *font_face, double font_size,
                            int item_height, int padding);
void menu_builder_set_activation_state(MenuBuilder *builder, uint16_t mod_key,
                                       uint8_t keycode);
void menu_config_destroy(MenuConfig *config);

/* Finalize the menu and create the Menu instance */
MenuConfig *menu_builder_finalize(MenuBuilder *builder);

/* Free the builder and internal state */
void menu_builder_destroy(MenuBuilder *builder);

#ifdef ENABLE_MENU_TOML
/* Load and construct menu from TOML file */
Menu *menu_builder_load_from_toml(const char *filename,
                                  void (*default_action)(void *));
#endif

#endif /* MENU_BUILDER_H */
