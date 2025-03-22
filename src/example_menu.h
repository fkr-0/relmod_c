/* example_menu.h */
#ifndef EXAMPLE_MENU_H
#define EXAMPLE_MENU_H

#include "input_handler.h"
#include "menu.h"

typedef struct ExampleMenuItem {
  uint32_t key;
  char *label;
  // ... other potential members
} ExampleMenuItem;

ExampleMenuItem *example_menu_add_item(MenuConfig *menu, const char *label,
                                       int (*command_func)(InputHandler *, int),
                                       uint32_t key);
int example_menu_item_command(InputHandler *handler, int item_id);
MenuConfig example_menu_create(uint16_t modifier_mask);
void example_menu_cleanup(void *user_data);

#endif
