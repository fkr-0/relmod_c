/* test_menu_manager.c - Unit tests for menu_manager */
#include "../src/menu.h"
#include "../src/menu_defaults.h"
#include "../src/menu_manager.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mocks */
static int triggered = 0;

static void dummy_action(void *user_data) { triggered++; }

static bool dummy_activates_cb(uint16_t mods, uint8_t key, void *user_data) {
  return key == 42;
}

static void test_creation_and_destruction() {
  MenuManager *mgr = menu_manager_create();
  assert(mgr != NULL);
  assert(menu_manager_get_menu_count(mgr) == 0);
  menu_manager_destroy(mgr);
}

static void test_register_and_find() {
  MenuManager *mgr = menu_manager_create();
  MenuConfig cfg = menu_config_default();
  cfg.title = "menu-1";
  Menu *menu = menu_create(&cfg);

  assert(menu_manager_register(mgr, menu) == true);
  assert(menu_manager_register(mgr, menu) == false); // duplicate
  assert(menu_manager_get_menu_count(mgr) == 1);
  assert(menu_manager_find_menu(mgr, "menu-1") == menu);

  menu_manager_unregister(mgr, menu);
  assert(menu_manager_get_menu_count(mgr) == 0);

  menu_destroy(menu);
  menu_manager_destroy(mgr);
}

static void test_activation_lifecycle() {
  MenuManager *mgr = menu_manager_create();
  MenuConfig cfg = menu_config_default();
  cfg.title = "menu-act";
  Menu *menu = menu_create(&cfg);
  assert(menu_manager_register(mgr, menu));

  assert(!menu_manager_activate(mgr, menu));
  assert(menu_manager_get_active(mgr) == menu);

  menu_manager_deactivate(mgr);
  assert(menu_manager_get_active(mgr) == NULL);

  menu_manager_unregister(mgr, menu);
  menu_destroy(menu);
  menu_manager_destroy(mgr);
}

/* static void test_key_event_activation(void) { */

/*   MenuManager *mgr = menu_manager_create(); */
/*   MenuConfig cfg = menu_config_default(); */
/*   cfg.title = "menu-key"; */
/*   cfg.act.custom_activate = NULL; */
/*   Menu *menu = menu_create(&cfg); */
/*   menu->activates_cb = dummy_activates_cb; */

/*   assert(menu_manager_register(mgr, menu)); */

/*   xcb_key_press_event_t event = {.detail = 42, .state = 0}; */
/*   /\* assert(menu_manager_handle_key_press(mgr, &event)); *\/ */
/*   assert(menu_manager_get_active(mgr) == menu); */

/*   menu_manager_deactivate(mgr); */
/*   menu_manager_unregister(mgr, menu); */
/*   menu_destroy(menu); */
/*   menu_manager_destroy(mgr); */
/* } */

static void test_status_string() {
  MenuManager *mgr = menu_manager_create();
  MenuConfig cfg = menu_config_default();
  cfg.title = "menu-status";
  Menu *menu = menu_create(&cfg);

  assert(menu_manager_register(mgr, menu));
  char *status = menu_manager_status_string(mgr);
  assert(status != NULL);
  assert(strstr(status, "menu-status") != NULL);
  free(status);

  menu_manager_unregister(mgr, menu);
  menu_destroy(menu);
  menu_manager_destroy(mgr);
}

int main() {
  test_creation_and_destruction();
  test_register_and_find();
  test_activation_lifecycle();
  /* test_key_event_activation(); */
  test_status_string();
  printf("All menu_manager tests passed.\n");
  return 0;
}
