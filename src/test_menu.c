#include <check.h>
#include <stdlib.h>
#include "menu.h"
#include "example_menu.h"
#include <xcb/xcb.h>

// Test activation logic
START_TEST(test_menu_activation) {
    MenuConfig config = example_menu_create(XCB_MOD_MASK_4);

    // Activate with correct modifier
    ck_assert(config.activates_cb(XCB_MOD_MASK_4, 0, config.user_data));

    // Don't activate with incorrect modifier
    ck_assert(!config.activates_cb(XCB_MOD_MASK_SHIFT, 0, config.user_data));

    // Test with combined modifiers
    ck_assert(config.activates_cb(XCB_MOD_MASK_4 | XCB_MOD_MASK_SHIFT, 0, config.user_data));

    // Test with no modifiers
    ck_assert(!config.activates_cb(0, 0, config.user_data));

    config.cleanup_cb(config.user_data);
}
END_TEST

// Test action logic
START_TEST(test_menu_action) {
    MenuConfig config = example_menu_create(XCB_MOD_MASK_4);

    // Test basic action
    ck_assert(config.action_cb(42, config.user_data));

    // Test multiple actions
    ck_assert(config.action_cb(43, config.user_data));
    ck_assert(config.action_cb(44, config.user_data));

    config.cleanup_cb(config.user_data);
}
END_TEST

// Test menu context creation and destruction
START_TEST(test_menu_context) {
    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    ck_assert(conn != NULL);
    
    xcb_ewmh_connection_t ewmh = {0};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL);

    X11FocusContext *focus_ctx = x11_focus_init(conn, &ewmh);
    ck_assert(focus_ctx != NULL);

    MenuConfig config = example_menu_create(XCB_MOD_MASK_4);
    MenuContext *menu_ctx = menu_create(focus_ctx, config);
    
    ck_assert(menu_ctx != NULL);
    ck_assert_int_eq(menu_ctx->active, false);
    ck_assert_int_eq(menu_ctx->active_modifier, 0);

    menu_destroy(menu_ctx);
    x11_focus_cleanup(focus_ctx);
    xcb_disconnect(conn);
}
END_TEST

// Test key event handling
START_TEST(test_menu_key_events) {
    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    xcb_ewmh_connection_t ewmh = {0};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL);

    X11FocusContext *focus_ctx = x11_focus_init(conn, &ewmh);
    MenuConfig config = example_menu_create(XCB_MOD_MASK_4);
    MenuContext *menu_ctx = menu_create(focus_ctx, config);
    
    // Set menu as active before testing key press
    menu_ctx->active = true;
    menu_ctx->active_modifier = XCB_MOD_MASK_4;

    // Test key press event
    xcb_key_press_event_t press_event = {
        .response_type = XCB_KEY_PRESS,
        .detail = 42,
        .state = XCB_MOD_MASK_4
    };
    ck_assert(menu_handle_key_press(menu_ctx, &press_event));

    // Test key release event
    xcb_key_release_event_t release_event = {
        .response_type = XCB_KEY_RELEASE,
        .detail = 42,
        .state = 0
    };
    menu_handle_key_release(menu_ctx, &release_event);

    menu_destroy(menu_ctx);
    x11_focus_cleanup(focus_ctx);
    xcb_disconnect(conn);
}
END_TEST

Suite *menu_suite(void) {
    Suite *s = suite_create("Menu");
    TCase *tc_core = tcase_create("Core");
    
    tcase_add_test(tc_core, test_menu_activation);
    tcase_add_test(tc_core, test_menu_action);
    tcase_add_test(tc_core, test_menu_context);
    tcase_add_test(tc_core, test_menu_key_events);
    
    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    int failed;
    Suite *s = menu_suite();
    SRunner *runner = srunner_create(s);
    srunner_run_all(runner, CK_NORMAL);
    failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

