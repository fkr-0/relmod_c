/* test_cairo_menu_render.c - Unit tests for Cairo menu rendering */
#include "../src/cairo_menu_render.h"
#include <assert.h>
#include <stdio.h>

/* Test rendering initialization */
void test_render_init() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(conn);

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t parent = screen->root;
  xcb_visualtype_t *visual = get_root_visual_type(conn);
  assert(visual);

  CairoMenuData data;
  bool result = cairo_menu_render_init(&data, conn, parent, visual);
  assert(result);

  cairo_menu_render_cleanup(&data);
  xcb_disconnect(conn);
}

/* Test rendering operations */
void test_render_operations() {
  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  assert(conn);

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t parent = screen->root;
  xcb_visualtype_t *visual = get_root_visual_type(conn);
  assert(visual);

  CairoMenuData data;
  cairo_menu_render_init(&data, conn, parent, visual);

  cairo_menu_render_begin(&data);
  cairo_menu_render_clear(&data,
                          &(MenuStyle){.background_color = {0, 0, 0, 1}});
  cairo_menu_render_end(&data);

  cairo_menu_render_cleanup(&data);
  xcb_disconnect(conn);
}

int main() {
  test_render_init();
  test_render_operations();
  printf("All tests passed.\n");
  return 0;
}
