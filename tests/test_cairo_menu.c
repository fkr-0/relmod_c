#include "../src/cairo_menu.h"
#include "../src/input_handler.h"
#include "../src/menu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

void test_menu_setup_cairo() {
  InputHandler *handler = input_handler_create();
  input_handler_setup_x(handler);
  xcb_connection_t *conn = handler->conn;
  assert(conn);

  xcb_screen_t *screen = handler->screen;
  assert(screen);

  xcb_window_t root = screen->root;

  MenuConfig config = {
      .mod_key = XCB_MOD_MASK_4,
      .trigger_key = 31,
      .title = "Test Menu",
      .items = NULL,
      .item_count = 0,
      .nav = {.next = {.key = 44, .label = "j"},
              .prev = {.key = 45, .label = "k"}},
      .act = {.activate_on_mod_release = true, .activate_on_direct_key = true},
      /* menu_setup_cairo(handler->conn, *handler->root,
         handler->focus_ctx,tests/test_performance.c:94: */
      /* Menu* menu = menu_setup_cairo(mock->conn, mock->root, &config); */
      .style = {.background_color = {0.1, 0.1, 0.1, 0.9},
                .text_color = {0.8, 0.8, 0.8, 1.0},
                .highlight_color = {0.3, 0.3, 0.8, 1.0},
                .font_face = "Sans",
                .font_size = 14.0,
                .item_height = 20,
                .padding = 10}};
  X11FocusContext *ctx = handler->focus_ctx;
  menu_setup_cairo(conn, root, ctx, screen, &config);
  assert(menu);

  menu_destroy(menu);
  input_handler_destroy(handler);
}

int main() {
  test_menu_setup_cairo();
  printf("All tests passed.\n");
  return 0;
}
