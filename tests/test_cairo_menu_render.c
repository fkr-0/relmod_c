/* test_cairo_menu_render.c - Unit tests for Cairo menu rendering */
#include "../src/cairo_menu_render.h"
#include <assert.h>
#include <stdio.h>
#include <xcb/xcb.h>

/* Forward declaration */
static xcb_visualtype_t *get_root_visual_type(xcb_connection_t *conn) {
  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

  for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
    xcb_visualtype_iterator_t visual_iter =
        xcb_depth_visuals_iterator(depth_iter.data);
    for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
      if (screen->root_visual == visual_iter.data->visual_id) {
        return visual_iter.data;
      }
    }
  }
  return NULL;
}

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
  printf("Test render init\n");
  test_render_init();
  printf("test render init done. \n test render operations.\n");
  test_render_operations();
  printf("All tests passed.\n");
  return 0;
}
