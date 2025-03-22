#include "cairo_menu.h"
#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

void test_cairo_menu_create() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(conn);

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  assert(screen);

  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 31,
      .title = "Test Menu",
      .items = NULL,
      .item_count = 0,
      .nav = {.next = {.key = 44, .label = "j"},
              .prev = {.key = 45, .label = "k"}},
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = true},
      .style = {.background_color = {0.1, 0.1, 0.1, 0.9},
                .text_color = {0.8, 0.8, 0.8, 1.0},
                .highlight_color = {0.3, 0.3, 0.8, 1.0},
                .font_face = "Sans",
                .font_size = 14.0,
                .item_height = 20,
                .padding = 10}};

  Menu *menu = cairo_menu_create(conn, screen->root, &config);
  assert(menu);

  menu_destroy(menu);
  xcb_disconnect(conn);
}

int main() {
  test_cairo_menu_create();
  printf("All tests passed.\n");
  return 0;
}
