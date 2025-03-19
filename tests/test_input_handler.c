#include <check.h>
#include <stdlib.h>
#include "../src/input_handler.h"
#include "../src/example_menu.h"
#include <xcb/xcb_ewmh.h>

// Helper function to setup X connection and window
static void setup_x_connection(xcb_connection_t **conn, xcb_ewmh_connection_t **ewmh, xcb_window_t *window) {
    *conn = xcb_connect(NULL, NULL);
    ck_assert_msg(*conn && !xcb_connection_has_error(*conn), "Failed to connect to X server");

    *ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(*conn, *ewmh);
    xcb_ewmh_init_atoms_replies(*ewmh, cookie, NULL);

    const xcb_setup_t *setup = xcb_get_setup(*conn);
    xcb_screen_t *screen = xcb_setup_roots_iterator(setup).data;
    *window = xcb_generate_id(*conn);

    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t values[2] = {
        screen->white_pixel,
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
    };

    xcb_create_window(*conn,
                    XCB_COPY_FROM_PARENT,
                    *window,
                    screen->root,
                    0, 0, 100, 100,  // x, y, width, height
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    mask, values);

    xcb_map_window(*conn, *window);
    xcb_flush(*conn);
}

// Test basic initialization and cleanup
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

// Test menu addition
START_TEST(test_input_handler_add_menu) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    InputHandler *handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    MenuConfig menu = example_menu_create(XCB_MOD_MASK_4);
    bool added = input_handler_add_menu(handler, menu);
    ck_assert_msg(added, "Failed to add menu to input handler");

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

// Test event handling
START_TEST(test_input_handler_event_flow) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    InputHandler *handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    MenuConfig menu = example_menu_create(XCB_MOD_MASK_4);
    input_handler_add_menu(handler, menu);

    // Test menu activation
    xcb_key_press_event_t fake_event_press = {
        .response_type = XCB_KEY_PRESS,
        .detail = 42,
        .state = XCB_MOD_MASK_4,
        .event = window,
        .root = window,
        .child = XCB_NONE,
        .same_screen = 1,
        .root_x = 0,
        .root_y = 0,
        .event_x = 0,
        .event_y = 0,
        .time = XCB_CURRENT_TIME
    };

    // Verify menu activation condition
    ck_assert(menu.activates_cb(fake_event_press.state, fake_event_press.detail, menu.user_data));

    // Test menu action
    ck_assert(menu.action_cb(fake_event_press.detail, menu.user_data));

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

// Test multiple menu addition
START_TEST(test_input_handler_multiple_menus) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    InputHandler *handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    // Add maximum number of menus
    for (int i = 0; i < MAX_MENUS; i++) {
        MenuConfig menu = example_menu_create(XCB_MOD_MASK_4 << i);
        bool added = input_handler_add_menu(handler, menu);
        ck_assert_msg(added, "Failed to add menu %d", i);
    }

    // Try to add one more menu (should fail)
    MenuConfig extra_menu = example_menu_create(XCB_MOD_MASK_4);
    bool added = input_handler_add_menu(handler, extra_menu);
    ck_assert(!added);

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

// Test menu activation sequences
START_TEST(test_input_handler_menu_activation) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    InputHandler *handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    // Add two menus with different modifiers
    MenuConfig menu1 = example_menu_create(XCB_MOD_MASK_4);
    MenuConfig menu2 = example_menu_create(XCB_MOD_MASK_SHIFT);

    input_handler_add_menu(handler, menu1);
    input_handler_add_menu(handler, menu2);

    // Test activation of first menu
    xcb_key_press_event_t event1 = {
        .response_type = XCB_KEY_PRESS,
        .detail = 42,
        .state = XCB_MOD_MASK_4,
        .event = window,
        .root = window,
        .child = XCB_NONE,
        .same_screen = 1
    };

    // Test activation of second menu
    xcb_key_press_event_t event2 = {
        .response_type = XCB_KEY_PRESS,
        .detail = 43,
        .state = XCB_MOD_MASK_SHIFT,
        .event = window,
        .root = window,
        .child = XCB_NONE,
        .same_screen = 1
    };

    ck_assert(menu1.activates_cb(event1.state, event1.detail, menu1.user_data));
    ck_assert(menu2.activates_cb(event2.state, event2.detail, menu2.user_data));

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

// Test error handling
START_TEST(test_input_handler_error_handling) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    // Test with NULL parameters
    InputHandler *handler = input_handler_create(NULL, NULL, XCB_NONE);
    ck_assert_msg(handler == NULL, "Should fail with NULL connection");

    // Test with invalid window
    handler = input_handler_create(conn, ewmh, XCB_NONE);
    ck_assert_msg(handler == NULL, "Should fail with invalid window");

    // Create valid handler for further tests
    handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    // Test adding NULL menu config
    MenuConfig invalid_config = {0};
    bool added = input_handler_add_menu(handler, invalid_config);
    ck_assert(!added);

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

// Test event loop handling
START_TEST(test_input_handler_event_loop) {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    xcb_window_t window;

    setup_x_connection(&conn, &ewmh, &window);

    InputHandler *handler = input_handler_create(conn, ewmh, window);
    ck_assert_msg(handler != NULL, "Failed to create input handler");

    // Add a menu
    MenuConfig menu = example_menu_create(XCB_MOD_MASK_4);
    input_handler_add_menu(handler, menu);

    // Create a synthetic key press event for ESC
    xcb_key_press_event_t esc_event = {
        .response_type = XCB_KEY_PRESS,
        .sequence = 0,
        .detail = 9,  // ESC key
        .state = 0,
        .event = window,
        .root = window,
        .child = XCB_NONE,
        .same_screen = 1,
        .root_x = 0,
        .root_y = 0,
        .event_x = 0,
        .event_y = 0,
        .time = XCB_CURRENT_TIME
    };

    // Send event with propagation enabled to ensure delivery
    xcb_void_cookie_t cookie = xcb_send_event_checked(conn, true, window,
                                                     XCB_EVENT_MASK_KEY_PRESS,
                                                     (char*)&esc_event);

    // Check if event was sent successfully
    xcb_generic_error_t *error = xcb_request_check(conn, cookie);
    ck_assert_msg(!error, "Failed to send event");
    if (error) {
        free(error);
    }

    xcb_flush(conn);

    // Give X server a moment to process the event
    struct timespec ts = {0, 10000000}; // 10ms
    nanosleep(&ts, NULL);

    // Process events until we get our escape key or timeout
    int max_attempts = 10;
    bool exit_requested = false;

    for (int i = 0; i < max_attempts && !exit_requested; i++) {
        exit_requested = input_handler_process_event(handler);
    }

    // Verify that the escape key triggered exit
    ck_assert_msg(exit_requested, "Escape key should trigger exit after %d attempts", max_attempts);

    input_handler_destroy(handler);
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(conn);
}
END_TEST

Suite *input_handler_suite(void) {
    Suite *s = suite_create("InputHandler");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_input_handler_init);
    tcase_add_test(tc, test_input_handler_add_menu);
    tcase_add_test(tc, test_input_handler_event_flow);
    tcase_add_test(tc, test_input_handler_multiple_menus);
    tcase_add_test(tc, test_input_handler_menu_activation);
    tcase_add_test(tc, test_input_handler_error_handling);
    tcase_add_test(tc, test_input_handler_event_loop);

    suite_add_tcase(s, tc);
    return s;
}

int main(void) {
    int failed;
    Suite *s = input_handler_suite();
    SRunner *runner = srunner_create(s);

    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
