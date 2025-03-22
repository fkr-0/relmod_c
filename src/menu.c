/* menu.c - Updated to delay dependency on X11FocusContext */

#include "menu.h"
#include <stdlib.h>
#include <string.h>

Menu *menu_create(MenuConfig *config) {
  if (!config)
    return NULL;
  Menu *menu = calloc(1, sizeof(Menu));
  if (!menu)
    return NULL;

  menu->config = *config;
  menu->state = MENU_STATE_INACTIVE;
  menu->selected_index = 0;
  menu->update_interval = 0;
  menu->user_data = config->act.user_data;
  menu->activates_cb =
      (bool (*)(uint16_t, uint8_t, void *))config->act.custom_activate;
  return menu;
}

void menu_set_focus_context(Menu *menu, X11FocusContext *ctx) {
  if (menu)
    menu->focus_ctx = ctx;
}

void menu_show(Menu *menu) {
  if (!menu || menu->active)
    return;
  menu->active = true;
  menu->state = MENU_STATE_INITIALIZING;
  if (menu->update_interval > 0 && menu->update_cb)
    menu_trigger_update(menu);
}

void menu_hide(Menu *menu) {
  if (!menu || !menu->active)
    return;
  menu->active = false;
  menu->state = MENU_STATE_INACTIVE;
  menu->selected_index = 0;
}

bool menu_handle_key_press(Menu *menu, xcb_key_press_event_t *ev) {
  if (!menu || !ev)
    return false;

  NavigationConfig *nav = &menu->config.nav;
  if (ev->detail == nav->next.key) {
    menu_select_next(menu);
    return true;
  }
  if (ev->detail == nav->prev.key) {
    menu_select_prev(menu);
    return true;
  }

  if (menu->config.act.activate_on_direct_key) {
    for (size_t i = 0; i < nav->direct.count; i++) {
      if (ev->detail == nav->direct.keys[i]) {
        menu_select_index(menu, i);
        return true;
      }
    }
  }

  return menu->action_cb ? menu->action_cb(ev->detail, menu->user_data) : false;
}

bool menu_handle_key_release(Menu *menu, xcb_key_release_event_t *ev) {
  if (!menu || !ev)
    return false;

  if (menu->config.act.activate_on_mod_release) {
    MenuItem *item = menu_get_selected_item(menu);
    if (item && item->action) {
      item->action(item->metadata);
      return false;
    }
  }
  return true;
}

void menu_destroy(Menu *menu) {
  if (!menu)
    return;
  if (menu->cleanup_cb)
    menu->cleanup_cb(menu->user_data);
  free(menu);
}

MenuItem *menu_get_selected_item(Menu *menu) {
  if (!menu || menu->selected_index < 0 ||
      menu->selected_index >= (int)menu->config.item_count)
    return NULL;
  return &menu->config.items[menu->selected_index];
}

void menu_select_next(Menu *menu) {
  if (!menu || menu->config.item_count == 0)
    return;
  menu->selected_index = (menu->selected_index + 1) % menu->config.item_count;
}

void menu_select_prev(Menu *menu) {
  if (!menu || menu->config.item_count == 0)
    return;
  menu->selected_index = (menu->selected_index + menu->config.item_count - 1) %
                         menu->config.item_count;
}

void menu_select_index(Menu *menu, int index) {
  if (!menu || index < 0 || (size_t)index >= menu->config.item_count)
    return;
  menu->selected_index = index;
}

bool menu_is_active(Menu *menu) { return menu ? menu->active : false; }

MenuState menu_get_state(Menu *menu) {
  return menu ? menu->state : MENU_STATE_INACTIVE;
}

void menu_set_update_interval(Menu *menu, unsigned int ms) {
  if (menu)
    menu->update_interval = ms;
}

void menu_set_update_callback(Menu *menu, void (*cb)(Menu *, void *)) {
  if (menu)
    menu->update_cb = cb;
}

void menu_trigger_update(Menu *menu) {
  if (menu && menu->update_cb)
    menu->update_cb(menu, menu->user_data);
}
