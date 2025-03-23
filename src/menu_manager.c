/* menu_manager.c - Menu management implementation */
#include "menu_manager.h"
#include "cairo_menu.h"
#include "cairo_menu_animation.h"
#include "cairo_menu_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[MANAGER]"
#include "log.h"
#endif

/* Registry entry */
typedef struct MenuRegistryEntry {
  Menu *menu;
  struct timeval last_update;
  struct MenuRegistryEntry *next;
} MenuRegistryEntry;

static void registry_cleanup(MenuRegistryEntry *entry) {
  while (entry) {
    MenuRegistryEntry *next = entry->next;
    menu_destroy(entry->menu);
    free(entry);
    entry = next;
  }
}

static MenuRegistryEntry *registry_find(MenuRegistryEntry *entry, Menu *menu) {
  while (entry) {
    if (entry->menu == menu)
      return entry;
    entry = entry->next;
  }
  return NULL;
}

static Menu *registry_find_by_id(MenuRegistryEntry *entry, const char *id) {
  while (entry) {
    if (entry->menu->config.title && strcmp(entry->menu->config.title, id) == 0)
      return entry->menu;
    entry = entry->next;
  }
  return NULL;
}

/* Registry iteration
 * Calls the given function for each registered menu
 * Stops iteration if the callback returns false.
 * Usage:
 * menu_manager_foreach(manager, callback_fn, user_data);
 * Example callback_fn:
 * bool callback_fn(Menu *menu, struct timeval *last_update, void *user_data) {
 *  // Do something with menu, e.g. match ev->state + ev->detail
 * if (menu->mod_key == ev->state && menu->trigger_key == ev->detail) {
 *   menu_manager_activate(manager, menu);
 *   return false; // Stop iteration
 * }
 * return true; // Continue iteration
 * */
void menu_manager_foreach(MenuManager *mgr, MenuManagerForEachFn fn,
                          void *user_data) {
  if (!mgr || !fn)
    return;
  MenuRegistryEntry *entry = (MenuRegistryEntry *)mgr->registry;
  while (entry) {
    if (!fn(entry->menu, &entry->last_update, user_data)) {
      break;
    }
    entry = entry->next;
  }
}

MenuManager *menu_manager_create() {
  MenuManager *mgr = calloc(1, sizeof(MenuManager));
  if (!mgr)
    return NULL;
  return mgr;
}

bool menu_manager_is_connected(MenuManager *mgr) {
  return mgr && mgr->conn && mgr->ewmh && mgr->focus_ctx;
}

MenuManager *menu_manager_connect(MenuManager *mgr, xcb_connection_t *conn,
                                  X11FocusContext *focus_ctx,
                                  xcb_ewmh_connection_t *ewmh) {
  if (!conn || !ewmh || !mgr) {
    fprintf(stderr, "Connecting MenuManager failed\n");
    return NULL;
  }
  mgr->conn = conn;
  mgr->ewmh = ewmh;
  mgr->focus_ctx = focus_ctx;
  return mgr;
}

void menu_manager_destroy(MenuManager *mgr) {
  if (!mgr)
    return;
  menu_hide(mgr->active_menu);
  registry_cleanup((MenuRegistryEntry *)mgr->registry);
  x11_focus_cleanup(mgr->focus_ctx);
  free(mgr);
}

bool menu_manager_register(MenuManager *mgr, Menu *menu) {
  if (!mgr || !menu)
    return false;
  MenuRegistryEntry *head = (MenuRegistryEntry *)mgr->registry;
  if (registry_find(head, menu)) {
    LOG("Menu already registered: [%s]", menu->config.title);
    return false;
  }

  MenuRegistryEntry *entry = calloc(1, sizeof(MenuRegistryEntry));
  if (!entry) {
    LOG("Failed to allocate MenuRegistryEntry\n");
    return false;
  }

  entry->menu = menu;
  gettimeofday(&entry->last_update, NULL);
  entry->next = head;
  mgr->registry = entry;
  mgr->menu_count++;
  LOG("Registered menu: [%s]", menu->config.title);
  return true;
}

void menu_manager_unregister(MenuManager *mgr, Menu *menu) {
  if (!mgr || !menu)
    return;
  if (mgr->active_menu == menu)
    menu_manager_deactivate(mgr);

  MenuRegistryEntry **prev = (MenuRegistryEntry **)&mgr->registry;
  while (*prev) {
    if ((*prev)->menu == menu) {
      MenuRegistryEntry *to_delete = *prev;
      *prev = to_delete->next;
      free(to_delete);
      mgr->menu_count--;
      LOG("Unregistered menu: %s", menu->config.title);
      return;
    }
    prev = &(*prev)->next;
  }
}

bool menu_manager_handle_key_press(MenuManager *mgr,
                                   xcb_key_press_event_t *event) {
  if (!mgr || !event) {
    LOG("Menu manager MGR or EVENT is null\n");
    return true;
  }
  if (mgr->active_menu) {
    LOG("Passing event to active menu %s", mgr->active_menu->config.title);
    return menu_handle_key_press(mgr->active_menu, event);
  } else {
    LOG("No active menu, checking registry\n");
  }

  MenuRegistryEntry *entry = (MenuRegistryEntry *)mgr->registry;
  while (entry) {
    if (entry->menu->activates_cb &&
        entry->menu->activates_cb(event->state, event->detail,
                                  entry->menu->user_data)) {
      LOG("Activating menu on key press/callback: %s",
          entry->menu->config.title);
      return menu_manager_activate(mgr, entry->menu);
    } else if (entry->menu->config.act.activate_on_mod_release &&
               entry->menu->config.mod_key == event->state) {
      LOG("Activating menu on mod: %s", entry->menu->config.title);
      return menu_manager_activate(mgr, entry->menu);
    }
    entry = entry->next;
  }
  return false;
}

bool menu_manager_handle_key_release(MenuManager *mgr,
                                     xcb_key_release_event_t *event) {
  if (!mgr || !event)
    return false;
  LOG("Key release %d %d %p", event->detail, event->state, mgr->active_menu);
  if (mgr->active_menu && !menu_handle_key_release(mgr->active_menu, event)) {
    menu_manager_deactivate(mgr);
    return true;
  }
  return false;
}
bool menu_manager_activate(MenuManager *mgr, Menu *menu) {
  if (!mgr || !menu)
    return true;

  if (!registry_find((MenuRegistryEntry *)mgr->registry, menu)) {
    fprintf(stderr, "Menu %s not registered\n", menu->config.title);
    return true;
  }

  LOG("Activating menu: %s", menu->config.title);
  // Deactivate any existing active menu
  if (mgr->active_menu && mgr->active_menu != menu) {
    LOG("Deactivating existing active menu: %s",
        mgr->active_menu->config.title);
    menu_manager_deactivate(mgr);
  }
  mgr->active_menu = menu;

  // Inject the X11FocusContext before showing the menu
  menu_set_focus_context(menu, mgr->focus_ctx);

  // Update timer reference
  MenuRegistryEntry *entry =
      registry_find((MenuRegistryEntry *)mgr->registry, menu);
  if (entry) {
    LOG("Updating last update time for menu: %s", menu->config.title);
    gettimeofday(&entry->last_update, NULL);
  } else {
    LOG("Failed to update last update time for menu: %s", menu->config.title);
  }
  LOG("Activating->Showing menu: %s", menu->config.title);
  menu_show(menu);
  LOG("Activated menu: %s", menu->config.title);
  /* cairo_menu_activate(menu); */
  /* cairo_menu_show(menu->user_data); */
  /* cairo_menu_render_show(menu->user_data); */
  /* cairo_menu_animation_show(menu->user_data); */

  return false;
}

void menu_manager_deactivate(MenuManager *mgr) {
  if (!mgr || !mgr->active_menu)
    return;
  LOG("Deactivated menu: %s", mgr->active_menu->config.title);
  menu_hide(mgr->active_menu);
  cairo_menu_deactivate(mgr->active_menu);
  mgr->active_menu = NULL;
}

Menu *menu_manager_get_active(MenuManager *mgr) {
  return mgr ? mgr->active_menu : NULL;
}

size_t menu_manager_get_menu_count(MenuManager *mgr) {
  return mgr ? mgr->menu_count : 0;
}

Menu *menu_manager_find_menu(MenuManager *mgr, const char *id) {
  return mgr && id ? registry_find_by_id((MenuRegistryEntry *)mgr->registry, id)
                   : NULL;
}

char *menu_manager_status_string(MenuManager *mgr) {
  if (!mgr)
    return NULL;
  char *buffer = calloc(1, 1024);
  if (!buffer)
    return NULL;

  snprintf(buffer, 1024, "Active: %s\nCount: %zu\n",
           mgr->active_menu ? mgr->active_menu->config.title : "None",
           mgr->menu_count);

  MenuRegistryEntry *entry = (MenuRegistryEntry *)mgr->registry;
  while (entry) {
    strcat(buffer, "Menu: ");
    strcat(buffer, entry->menu->config.title);
    strcat(buffer, "\n");
    entry = entry->next;
  }

  return buffer;
}
