/* test_cairo_menu_animation.c - Unit tests for Cairo menu animation */
#include "../src/cairo_menu_animation.h"
#include <assert.h>
#include <stdio.h>

/* Test animation initialization */
void test_animation_init() {
  printf("Starting test_animation_init\n"); // Add debug print
  CairoMenuData data;
  cairo_menu_animation_init(&data);

  assert(data.anim.show_animation);
  assert(data.anim.hide_animation);

  cairo_menu_animation_cleanup(&data);
  printf("Finished test_animation_init\n"); // Add debug print
}

/* Test animation updates */
void test_animation_update() {
  printf("Starting test_animation_update\n"); // Add debug print
  CairoMenuData data;
  cairo_menu_animation_init(&data);

  Menu menu;
  menu.state = MENU_STATE_INITIALIZING;

  struct timeval now;
  gettimeofday(&now, NULL);
  data.anim.last_frame = now;

  cairo_menu_animation_show(&data, &menu);
  assert(data.anim.is_animating);

  double delta = 16.0; /* Simulate 16ms frame time */
  cairo_menu_animation_update(&data, &menu, delta);

  cairo_menu_animation_hide(&data, &menu);
  assert(data.anim.is_animating);

  cairo_menu_animation_update(&data, &menu, delta);

  cairo_menu_animation_cleanup(&data);
  printf("Finished test_animation_update\n"); // Add debug print
}

int main() {
  printf("Starting tests\n"); // Add debug print
  test_animation_init();
  test_animation_update();
  printf("All tests passed.\n");
  return 0;
}
