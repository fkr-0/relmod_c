/* menu.c - Updated to delay dependency on X11FocusContext */

#include "menu.h"
#include "cairo_menu.h"
#include "cairo_menu_animation.h"
#include "cairo_menu_render.h"
#include "menu_animation.h"
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

#include "log.h"
#ifdef MENU_DEBUG
#define LOG_PREFIX "[CAIRO_MENU]"
#endif

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
  /* menu->user_data = config->act.user_data; */
  /* menu->activates_cb = */
  /*     (bool (*)(uint16_t, uint8_t, void *))config->act.custom_activate; */
  return menu;
}

void menu_set_focus_context(Menu *menu, X11FocusContext *ctx) {
  if (menu)
    menu->focus_ctx = ctx;
}

void menu_show(Menu *menu) {
  if (!menu || menu->active) {
    LOG("Menu is already active or NULL [%p]", menu, menu->active);
    return;
  }
  menu->active = true;
  menu->state = MENU_STATE_INITIALIZING;
  LOG("Menu is inactive, setting state to initializing");

  CairoMenuData *data = (CairoMenuData *)menu->user_data;
  if (!data) {
    LOG("No user data found");
    return;
  }
  cairo_menu_animation_init(data);
  /* LOG("Animations initialized successfully\n"); */
  menu_set_update_interval(menu, 20);
  cairo_menu_animation_set_default(data, MENU_ANIM_FADE, MENU_ANIM_FADE,
                                   40000.0);

  /* cairo_menu_render_scale(data, 1.0, 1.0); */
  cairo_menu_activate(menu);
  /* cairo_menu_render_set_opacity(data, 0.5); */
  /* cairo_menu_animation_show(data, menu); */
  /* menu_animation_create(MENU_ANIM_FADE, 4000.0); */
  /* MenuAnimationSequence *seq = menu_animation_sequence_create(); */
  /* menu_animation_sequence_add(seq, menu_animation_fade_in(40000)); */
  /* cairo_menu_animation_set_sequence(data, true, seq); */
  /* cairo_menu_animation_update(data, menu, 400000); */
  cairo_menu_render_show(data);
  /* menu->state = MENU_STATE_ACTIVE; */
  cairo_menu_show(menu);
  /* cairo_menu_render_request_update(data); */
  /* menu_trigger_update(menu); */
  /* cairo_menu_animation_apply(data, menu, data->render.cr); */
  /* data->anim.show_animation = menu_animation_fade_in(8000); */
  /* cairo_menu_animation_show(data, menu); */
  /* menu_trigger_update(menu); */
  /* /\* menu->state = MENU_STATE_ACTIVE; *\/ */
  /* menu_trigger_update(menu); */
  /* menu_redraw(menu); */
  /* } */
  if (menu->update_interval > 0 && menu->update_cb)
    menu_trigger_update(menu);
}

void menu_hide(Menu *menu) {
  /* if (!menu || !menu->active) */
  /*   return; */
  /* menu->active = false; */
  /* menu->state = MENU_STATE_INACTIVE; */
  /* menu->selected_index = 0; */
  if (menu_cairo_is_setup(menu)) {
    cairo_menu_deactivate(menu);
  }
}

bool menu_handle_key_press(Menu *menu, xcb_key_press_event_t *ev) {
  if (!menu || !ev)
    return true;

  NavigationConfig *nav = &menu->config.nav;
  LOG("Key press %d %d [%s]", ev->detail, ev->state, menu->config.title);
  LOG("NAV: %d %d %d Trigger %d", nav->next.key, nav->prev.key,
      nav->direct.count, menu->config.trigger_key);
  if (ev->detail == nav->next.key || ev->detail == menu->config.trigger_key) {
    LOG("Selecting next item");
    menu_select_next(menu);
    return false;
  }
  if (ev->detail == nav->prev.key) {
    LOG("Selecting previous item");
    menu_select_prev(menu);
    return false;
  }
  // keycode 1-9: int 10-18
  if (ev->detail >= 10 && ev->detail <= 18) {
    if (ev->detail - 10 < (int)menu->config.item_count) {
      LOG("Selecting item by direct key");
      menu_select_index(menu, ev->detail - 10);
      return false;
    } else {
      LOG("Invalid direct key");
    }
    return false;
  }
  if (menu->config.act.activate_on_direct_key) {
    for (size_t i = 0; i < nav->direct.count; i++) {
      if (ev->detail == nav->direct.keys[i]) {
        menu_select_index(menu, i);
        return false;
      }
    }
  }

  return menu->action_cb ? menu->action_cb(ev->detail, menu->user_data) : false;
}

void menu_confirm_selection(Menu *menu) {
  MenuItem *item = menu_get_selected_item(menu);
  if (item && item->action) {

    // Check if the item's action is valid
    // Prepare the data for the action
    const char *d = (const char *)item->metadata;
    void *data = d ? (void *)d : menu->user_data;

    // Log the action call
    LOG("Calling action for item: %s", d ? d : "no metadata");

    // Execute the action
    item->action(data);

    // Log the action completion
    LOG("Action called");

  } else
    LOG("Action not called");
}

void menu_destroy(Menu *menu) {
  if (!menu)
    return;
  if (menu->cleanup_cb)
    menu->cleanup_cb(menu->user_data);
  /* if (menu->user_data) */
  /*   free(menu->user_data); */
  /* if (menu->config.title) */
  /*   free(menu->config.title); */
  if (menu->config.nav.direct.keys)
    free(menu->config.nav.direct.keys);
  if (menu->config.items)
    free(menu->config.items);

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
  LOG("[%s][%d/%d] Select next", menu->config.title, menu->selected_index,
      menu->config.item_count);

  menu_trigger_on_select(menu);
  menu_trigger_update(menu);
}

void menu_select_prev(Menu *menu) {
  if (!menu || menu->config.item_count == 0)
    return;
  menu->selected_index = (menu->selected_index + menu->config.item_count - 1) %
                         menu->config.item_count;
  LOG("[%s][%d/%d] Select prev", menu->config.title, menu->selected_index,
      menu->config.item_count);

  menu_trigger_on_select(menu);
  menu_trigger_update(menu);
}

void menu_select_index(Menu *menu, int index) {
  if (!menu || index < 0 || (size_t)index >= menu->config.item_count ||
      menu->selected_index == index)
    return;
  menu->selected_index = index;

  menu_trigger_on_select(menu);
  menu_trigger_update(menu);
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

void menu_set_activation_state(MenuConfig *config, int mod_key, int keycode) {
  if (!config)
    return;

  /* if (!menu->config.act_state.initialized) { */
  config->act_state.config = config;
  config->act_state.mod_key = mod_key;
  config->act_state.keycode = keycode;
  config->act_state.initialized = false;
  /* ActivationState *state = &config->act_state; */
  /* LOG("State: config=%p, mod_key=0x%x, keycode=%u, initialized=%d, " */
  /*     "menu=%p, title=%s", */
  /*     state->config, state->mod_key, state->keycode, state->initialized, */
  /*     state->menu, state->config->title); */

  /* config->act_state.menu = menu; */
  /* } else { */
  /*   menu->config.act_state.mod_key = mod_key; */
  /*   menu->config.act_state.keycode = keycode; */
  /*   menu->config.act_state.menu = menu; */
  /* } */
}

void menu_redraw(Menu *menu) {
  if (!menu)
    return;
  cairo_menu_render_request_update(menu->user_data);
}

void menu_set_on_select_callback(Menu *menu,
                                 void (*on_select)(MenuItem *item,
                                                   void *user_data)) {
  if (menu) {
    menu->on_select = on_select;
  }
}

void menu_trigger_on_select(Menu *menu) {
  if (menu && menu->on_select) {
    MenuItem *item = menu_get_selected_item(menu);
    if (item) {
      menu->on_select(item, menu->user_data);
    }
  }
}
