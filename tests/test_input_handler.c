#include "../src/example_menu.h"
#include "../src/input_handler.h"
#include <check.h>
#include <stdlib.h>
#include <xcb/xcb_ewmh.h>

// Helper function to setup X connection and window
static void setup_x_connection(xcb_connection_t **conn,
                               xcb_ewmh_connection_t **ewmh,
                               xcb_window_t *window) {
  *conn = xcb_connect(NULL, NULL);
  ck_assert_msg(*conn && !xcb_connection_has_error(*conn),
                "Failed to connect to X server");

  *ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(*conn, *ewmh);
  xcb_ewmh_init_atoms_replies(*ewmh, cookie, NULL);

  const xcb_setup_t *setup = xcb_get_setup(*conn);
  xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;
  *window = xcb_generate_id(*conn);

  uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t values[2] = {screen->white_pixel,
                        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE};

  xcb_create_window(*conn, XCB_COPY_FROM_PARENT, *window, screen->root, 0, 0,
                    100, 100, // x, y, width, height
                    0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask,
                    values);

  xcb_map_window(*conn, *window);
  xcb_flush(*conn);
}

START_TEST(test_input_handler_init) {
  xcb_connection_t *conn;
  xcb_ewmh_connection_t *ewmh;
  xcb_window_t window;

  setup_x_connection(&conn, &ewmh, &window);
  InputHandler *handler = input_handler_create(conn, ewmh, window);
  ck_assert_msg(handler != NULL, "Failed to create input handler");

  input_handler_destroy(handler);
  xcb_ewmh_connection_wipe(ewmh);
  free(ewmh);
  xcb_disconnect(conn);
}
END_TEST

START_TEST(test_input_handler_add_menu) {
  xcb_connection_t *conn;
  xcb_ewmh_connection_t *ewmh;
  xcb_window_t window;
  setup_x_connection(&conn, &ewmh, &window);
  InputHandler *handler = input_handler_create(conn, ewmh, window);
  ck_assert_msg(handler != NULL, "Failed to create input handler");
  MenuConfig menu_config = example_menu_create(XCB_MOD_MASK_4);
  bool added = input_handler_add_menu(handler, menu_config);
  ck_assert_msg(added, "Failed to add menu to input handler");
  input_handler_destroy(handler);
  xcb_ewmh_connection_wipe(ewmh);
  free(ewmh);
  xcb_disconnect(conn);
}
END_TEST

START_TEST(test_input_handler_remove_menu) {
  xcb_connection_t *conn;
  xcb_ewmh_connection_t *ewmh;
  xcb_window_t window;
  setup_x_connection(&conn, &ewmh, &window);
  InputHandler *handler = input_handler_create(conn, ewmh, window);
  ck_assert_msg(handler != NULL, "Failed to create input handler");
  MenuConfig menu_config = example_menu_create(XCB_MOD_MASK_4);
  input_handler_add_menu(handler, menu_config);
  bool removed = input_handler_remove_menu(handler, menu_config);
  ck_assert_msg(removed, "Failed to remove menu from input handler");
  input_handler_destroy(handler);
  xcb_ewmh_connection_wipe(ewmh);
  free(ewmh);
  xcb_disconnect(conn);
}
END_TEST

START_TEST(test_input_handler_handle_key) {
  xcb_connection_t *conn;
  xcb_ewmh_connection_t *ewmh;
  xcb_window_t window;
  InputHandler *handler;
  MenuConfig menu_config;

  setup_x_connection(&conn, &ewmh, &window);
  handler = input_handler_create(conn, ewmh, window);
  ck_assert_msg(handler != NULL, "Failed to create input handler");

  menu_config = example_menu_create(XCB_MOD_MASK_4);
  input_handler_add_menu(handler, menu_config);

  ExampleMenuItem *item = example_menu_add_item(
      &menu_config, "Test Item", example_menu_item_command(handler, 0), 0);
  ck_assert_msg(input_handler_process_event(handler),
                "Failed to handle key press");

  input_handler_destroy(handler);

  xcb_ewmh_connection_wipe(ewmh);
  free(ewmh);
  xcb_disconnect(conn);
}
END_TEST

Suite *input_handler_suite(void) {
  Suite *s;
  TCase *tc_core;

  s = suite_create("Input Handler");
  tc_core = tcase_create("Core");

  tcase_add_test(tc_core, test_input_handler_init);
  tcase_add_test(tc_core, test_input_handler_add_menu);
  tcase_add_test(tc_core, test_input_handler_remove_menu);
  tcase_add_test(tc_core, test_input_handler_handle_key);

  suite_add_tcase(s, tc_core);

  return s;
}

int main(void) {
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = input_handler_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
// Additional unit tests for src/input_handler.c
