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

/* Finalize the menu and create the Menu instance */
Menu *menu_builder_finalize(MenuBuilder *builder);

/* Free the builder and internal state */
void menu_builder_destroy(MenuBuilder *builder);

#ifdef ENABLE_MENU_TOML
/* Load and construct menu from TOML file */
Menu *menu_builder_load_from_toml(const char *filename,
                                  void (*default_action)(void *));
#endif

#endif /* MENU_BUILDER_H */
