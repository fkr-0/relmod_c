/* menu_manager.c - Menu management implementation */
#include "menu_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

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

MenuManager *menu_manager_create(xcb_connection_t *conn,
                                 xcb_ewmh_connection_t *ewmh) {
  if (!conn || !ewmh)
    return NULL;
  MenuManager *mgr = calloc(1, sizeof(MenuManager));
  if (!mgr)
    return NULL;
  mgr->conn = conn;
  mgr->ewmh = ewmh;
  mgr->focus_ctx = x11_focus_init(conn, ewmh);
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
  if (registry_find(head, menu))
    return false;

  MenuRegistryEntry *entry = calloc(1, sizeof(MenuRegistryEntry));
  if (!entry)
    return false;

  entry->menu = menu;
  gettimeofday(&entry->last_update, NULL);
  entry->next = head;
  mgr->registry = entry;
  mgr->menu_count++;
  LOG("Registered menu: %s", menu->config.title);
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
  if (!mgr || !event)
    return false;
  if (mgr->active_menu)
    return menu_handle_key_press(mgr->active_menu, event);

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
  if (mgr->active_menu && !menu_handle_key_release(mgr->active_menu, event)) {
    menu_manager_deactivate(mgr);
    return true;
  }
  return false;
}
bool menu_manager_activate(MenuManager *mgr, Menu *menu) {
  if (!mgr || !menu)
    return false;

  if (!registry_find((MenuRegistryEntry *)mgr->registry, menu)) {
    fprintf(stderr, "Menu not registered\n");
    return false;
  }

  // Deactivate any existing active menu
  if (mgr->active_menu)
    menu_manager_deactivate(mgr);

  // Inject the X11FocusContext before showing the menu
  menu_set_focus_context(menu, mgr->focus_ctx);

  // Update timer reference
  MenuRegistryEntry *entry =
      registry_find((MenuRegistryEntry *)mgr->registry, menu);
  if (entry)
    gettimeofday(&entry->last_update, NULL);

  menu_show(menu);
  mgr->active_menu = menu;
  return true;
}

void menu_manager_deactivate(MenuManager *mgr) {
  if (!mgr || !mgr->active_menu)
    return;
  LOG("Deactivated menu: %s", mgr->active_menu->config.title);
  menu_hide(mgr->active_menu);
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
