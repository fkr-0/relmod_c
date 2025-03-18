/* example_menu.h */
#ifndef EXAMPLE_MENU_H
#define EXAMPLE_MENU_H

#include "menu.h"

MenuConfig example_menu_create(uint16_t modifier_mask);
void example_menu_cleanup(void *user_data);

#endif
