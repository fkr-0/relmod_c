/* menu.c - Updated to delay dependency on X11FocusContext */

#include "menu.h"
#include "cairo_menu.h"
#include "cairo_menu_animation.h"
#include "cairo_menu_render.h"
#include "menu_animation.h"
#include "x11_window.h"
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[CAIRO_MENU]"
#endif
#include "log.h"

Menu *menu_create(MenuConfig *config) {
    if (!config) return NULL;

    Menu *menu = calloc(1, sizeof(Menu));
    if (!menu) {
        perror("Failed to allocate Menu struct");
        return NULL;
    }

    // --- Perform a deep copy of the configuration ---

    // Copy simple fields
    menu->config.mod_key = config->mod_key;
    menu->config.trigger_key = config->trigger_key;
    menu->config.title = config->title ? strdup(config->title) : NULL; // Duplicate title
    menu->config.item_count = config->item_count;
    menu->config.nav = config->nav; // Struct copy is fine for nav (labels are const char*)
                                    // NOTE: If nav.direct.keys was dynamically allocated in builder,
                                    // this shallow copy might be insufficient if config is freed later.
                                    // However, menu_config_destroy frees config->nav.direct.keys,
                                    // so menu->config.nav.direct.keys becomes dangling.
                                    // Let's deep copy direct keys too.
    menu->config.act = config->act; // Struct copy is fine for act
    menu->config.style = config->style; // Struct copy is fine for style

    // Deep copy direct navigation keys if they exist
    if (config->nav.direct.count > 0 && config->nav.direct.keys) {
        menu->config.nav.direct.keys = calloc(config->nav.direct.count, sizeof(uint8_t));
        if (!menu->config.nav.direct.keys) {
             perror("Failed to allocate direct nav keys in menu_create");
             free((void*)menu->config.title);
             free(menu);
             return NULL;
        }
        memcpy(menu->config.nav.direct.keys, config->nav.direct.keys, config->nav.direct.count * sizeof(uint8_t));
        // menu->config.nav.direct.count was already copied by struct copy
    } else {
        menu->config.nav.direct.keys = NULL;
        menu->config.nav.direct.count = 0;
    }


    // Deep copy items array
    if (config->item_count > 0 && config->items) {
        menu->config.items = calloc(config->item_count, sizeof(MenuItem));
        if (!menu->config.items) {
            perror("Failed to allocate items array in menu_create");
            free((void*)menu->config.title);
            free(menu->config.nav.direct.keys); // Free copied direct keys
            free(menu);
            return NULL;
        }

        for (size_t i = 0; i < config->item_count; i++) {
            MenuItem *src_item = &config->items[i];
            MenuItem *dst_item = &menu->config.items[i];

            // Duplicate id string
            dst_item->id = src_item->id ? strdup(src_item->id) : NULL;
            if (src_item->id && !dst_item->id) { // Check if strdup failed
                 perror("Failed to duplicate item id in menu_create");
                 // Cleanup partially allocated items in menu->config.items
                 for(size_t j = 0; j < i; ++j) { free((void*)menu->config.items[j].id); free((void*)menu->config.items[j].label); }
                 free(menu->config.items);
                 free((void*)menu->config.title);
                 free(menu->config.nav.direct.keys);
                 free(menu);
                 return NULL;
            }

            // Duplicate label string
            dst_item->label = src_item->label ? strdup(src_item->label) : NULL;
             if (src_item->label && !dst_item->label) { // Check if strdup failed
                 perror("Failed to duplicate item label in menu_create");
                 // Cleanup partially allocated items in menu->config.items
                 free((void*)dst_item->id); // Free the id we just duped for item 'i'
                 for(size_t j = 0; j < i; ++j) { free((void*)menu->config.items[j].id); free((void*)menu->config.items[j].label); }
                 free(menu->config.items);
                 free((void*)menu->config.title);
                 free(menu->config.nav.direct.keys);
                 free(menu);
                 return NULL;
            }

            // Copy action callback pointer
            dst_item->action = src_item->action;
            // Copy metadata pointer (ownership does NOT transfer)
            dst_item->metadata = src_item->metadata;
        }
    } else {
        menu->config.items = NULL;
        menu->config.item_count = 0; // Ensure count is 0 if items is NULL
    }

    // --- Initialize Menu state ---
    menu->state = MENU_STATE_INACTIVE;
    menu->selected_index = 0;
    menu->update_interval = 0; // Default, can be set later
    menu->user_data = NULL;    // Should be set explicitly after creation if needed
    menu->cleanup_cb = NULL;   // Should be set explicitly
    menu->update_cb = NULL;    // Should be set explicitly
    menu->on_select = NULL;    // Should be set explicitly
    menu->action_cb = NULL;    // Should be set explicitly
    menu->focus_ctx = NULL;    // Set via menu_set_focus_context

    // Note: config->act_state is not copied deeply here. It's assumed
    // menu activation logic will handle initializing the state within the Menu struct later.

    // IMPORTANT: menu_create does NOT free the input 'config' pointer or its contents.
    // The caller is responsible for calling menu_config_destroy(config) after menu_create returns.

    return menu;
}

void menu_set_focus_context(Menu *menu, X11FocusContext *ctx) {
  if (menu)
    menu->focus_ctx = ctx;
}

void menu_show(Menu *menu) {
  if (!menu || menu->active) {
    LOG("Menu %p is already active or NULL [%p]", &menu, &menu->active);
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
  /* menu_set_update_interval(menu, 20); */
  /* cairo_menu_animation_set_default(data, MENU_ANIM_FADE, MENU_ANIM_FADE, */
  /*                                  40000.0); */

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
  /* if (menu->update_interval > 0 && menu->update_cb) */
  /*   menu_trigger_update(menu); */
  menu_trigger_on_select(menu);
}

void menu_hide(Menu *menu) {
  /* if (!menu || !menu->active) */
  /*   return; */
  menu->active = false;
  menu->state = MENU_STATE_INACTIVE;
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
  LOG("NAV: %d %d %ld Trigger %d", nav->next.key, nav->prev.key,
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
void menu_cancel(Menu *menu) {
  if (!menu || !menu->active)
    return;
  menu->active = false;
  menu->state = MENU_STATE_INACTIVE;
  menu->selected_index = 0;
  xcb_window_t win = menu->focus_ctx->previous_focus;
  if (win) {
    window_activate(menu->focus_ctx->conn, win);
    switch_to_window(menu->focus_ctx->conn, win);
  }
  /* cairo_menu_hide(menu); */
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
    if (!menu) return;

    // 1. Call user-provided cleanup callback first (if any)
    // This callback might free menu->user_data or other resources associated with it.
    if (menu->cleanup_cb) {
        menu->cleanup_cb(menu->user_data);
        // Note: The cleanup_cb should NOT free menu->user_data if it's managed
        // by the Cairo backend (like CairoMenuData). That's handled below.
    }

    // 2. Clean up Cairo-specific data if it exists and is managed here
    // The user_data (e.g., CairoMenuData) should be cleaned up *only* by the
    // cleanup_cb assigned during setup (e.g., cairo_menu_cleanup assigned in menu_setup_cairo).
    // The cleanup_cb is responsible for freeing the user_data struct itself.
    // Therefore, no separate call to cairo_menu_destroy is needed here.
    // The check below is redundant if cleanup_cb handles it correctly.
    /*
    if (menu->user_data && menu_cairo_is_setup(menu)) {
         // This block is removed as cleanup_cb handles freeing user_data.
         // Calling cairo_menu_destroy here would lead to a double free if
         // cleanup_cb is also set to cairo_menu_cleanup.
         // cairo_menu_destroy(menu); // REMOVED
         // menu->user_data = NULL; // cleanup_cb should handle this if it frees user_data
    }
    */
    // If user_data was something else and not freed by cleanup_cb, it's leaked.

    // 3. Free resources allocated by menu_create within menu->config
    free((void*)menu->config.title); // Free duplicated title
    if (menu->config.items) {
        for (size_t i = 0; i < menu->config.item_count; i++) {
            free((void*)menu->config.items[i].id);    // Free duplicated id
            free((void*)menu->config.items[i].label); // Free duplicated label
            // DO NOT free menu->config.items[i].metadata here - ownership belongs elsewhere
        }
        free(menu->config.items); // Free the items array itself
    }
    free(menu->config.nav.direct.keys); // Free duplicated direct nav keys

    // 4. Free the Menu struct itself
    free(menu);
}

MenuItem *menu_get_selected_item(Menu *menu) {
  if (!menu || menu->selected_index < 0 ||
      menu->selected_index >= (int)menu->config.item_count)
    return NULL;
  return &menu->config.items[menu->selected_index];
}

void menu_select_next(Menu *menu) {
    if (!menu || menu->config.item_count == 0) return; // Prevent division by zero
    menu_select_index(menu, (menu->selected_index + 1) % menu->config.item_count);
}

void menu_select_prev(Menu *menu) {
    if (!menu || menu->config.item_count == 0) return; // Prevent issues with count 0
    menu_select_index(menu,
                      (menu->selected_index == 0 ? menu->config.item_count - 1
                                                 : menu->selected_index - 1));
}

void menu_select_index(Menu *menu, int index) {
  LOG("Selected index: %d count: %d index: %d", menu->selected_index,
      menu->config.item_count, index);
  if (!menu || menu->config.item_count == 0 || index < 0 ||
      (size_t)index >= menu->config.item_count || menu->selected_index == index)
    return;
  menu->selected_index = index;
  menu_trigger_on_select(menu);
  LOG("Selected index: %d", menu->selected_index);
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

void menu_redraw(Menu *menu) {
  if (!menu)
    return;
  // Only redraw if Cairo data exists (i.e., Cairo backend is set up)
  if (menu->user_data) {
      cairo_menu_render_request_update(menu->user_data);
      cairo_menu_render_show(menu->user_data);
  }
}

void menu_set_on_select_callback(Menu *menu,
                                 void (*on_select)(MenuItem *item,
                                                   void *user_data)) {
  if (menu) {
    menu->on_select = on_select;
  }
}

void menu_trigger_on_select(Menu *menu) {
  MenuItem *item = menu_get_selected_item(menu); // Get item first
  LOG("Triggering on select menu=%p with item=%p, user_data=%p, callback=%p",
      (void*)menu, (void*)item, (void*)menu->user_data, (void*)menu->on_select);
  if (menu && menu->on_select) {
    // Item already fetched above
    LOG("Triggering with item %p and data %p", item, menu->user_data);
    if (item) {
      menu->on_select(item, menu->user_data);
    }
    LOG("DONE Triggering with item %p and data %p", item, menu->user_data);
    menu_redraw(menu);
  }
}
