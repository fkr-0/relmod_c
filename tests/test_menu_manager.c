/* test_menu_manager.c - Unit tests for menu manager functionality */
#include "../src/menu_manager.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>

/* Mock X11 environment */
typedef struct {
    xcb_connection_t* conn;
    xcb_window_t root;
    xcb_screen_t* screen;
    xcb_ewmh_connection_t ewmh;
} MockX11;

/* Test state tracking */
static int action_count = 0;
static Menu* last_activated_menu = NULL;

/* Mock action callback */
static void test_action(void* user_data) {
    action_count++;
    (void)user_data;
}

/* Initialize mock X11 environment */
static MockX11 setup_mock_x11(void) {
    MockX11 mock = {0};
    mock.conn = xcb_connect(NULL, NULL);
    assert(!xcb_connection_has_error(mock.conn));
    
    mock.screen = xcb_setup_roots_iterator(xcb_get_setup(mock.conn)).data;
    mock.root = mock.screen->root;
    
    xcb_intern_atom_cookie_t* cookie = xcb_ewmh_init_atoms(mock.conn, &mock.ewmh);
    assert(xcb_ewmh_init_atoms_replies(&mock.ewmh, cookie, NULL));
    
    return mock;
}

/* Clean up mock environment */
static void cleanup_mock_x11(MockX11* mock) {
    xcb_ewmh_connection_wipe(&mock->ewmh);
    xcb_disconnect(mock->conn);
}

/* Create a test menu */
static Menu* create_test_menu(X11FocusContext* focus_ctx, const char* id, uint16_t mod, uint8_t trigger) {
    static MenuItem items[] = {
        { .id = "1", .label = "Test 1", .action = test_action },
        { .id = "2", .label = "Test 2", .action = test_action }
    };
    
    MenuConfig config = {
        .mod_key = mod,
        .trigger_key = trigger,
        .title = id,
        .items = items,
        .item_count = 2,
        .nav = {
            .next = { .key = 44, .label = "j" },
            .prev = { .key = 45, .label = "k" },
            .direct = {
                .keys = (uint8_t[]){10, 11},
                .count = 2
            }
        },
        .act = {
            .activate_on_mod_release = true,
            .activate_on_direct_key = true
        }
    };
    
    return menu_create(focus_ctx, &config);
}

/* Test menu manager creation/destruction */
static void test_manager_lifecycle(void) {
    printf("Testing menu manager lifecycle...\n");
    
    MockX11 mock = setup_mock_x11();
    
    /* Create manager */
    MenuManager* manager = menu_manager_create(mock.conn, &mock.ewmh);
    assert(manager != NULL);
    assert(manager->menu_count == 0);
    assert(manager->active_menu == NULL);
    
    /* Clean up */
    menu_manager_destroy(manager);
    cleanup_mock_x11(&mock);
    
    printf("Menu manager lifecycle test passed\n");
}

/* Test menu registration */
static void test_menu_registration(void) {
    printf("Testing menu registration...\n");
    
    MockX11 mock = setup_mock_x11();
    MenuManager* manager = menu_manager_create(mock.conn, &mock.ewmh);
    assert(manager != NULL);
    
    /* Create test menus */
    Menu* menu1 = create_test_menu(manager->focus_ctx, "Menu 1", 
                                  XCB_MOD_MASK_4, 31);  // Super+i
    Menu* menu2 = create_test_menu(manager->focus_ctx, "Menu 2",
                                  XCB_MOD_MASK_4, 32);  // Super+o
    
    /* Test registration */
    assert(menu_manager_register(manager, menu1));
    assert(manager->menu_count == 1);
    
    assert(menu_manager_register(manager, menu2));
    assert(manager->menu_count == 2);
    
    /* Test duplicate registration */
    assert(!menu_manager_register(manager, menu1));
    assert(manager->menu_count == 2);
    
    /* Test finding menus */
    assert(menu_manager_find_menu(manager, "Menu 1") == menu1);
    assert(menu_manager_find_menu(manager, "Menu 2") == menu2);
    assert(menu_manager_find_menu(manager, "NonExistent") == NULL);
    
    /* Clean up */
    menu_manager_destroy(manager);
    cleanup_mock_x11(&mock);
    
    printf("Menu registration test passed\n");
}

/* Test menu activation */
static void test_menu_activation(void) {
    printf("Testing menu activation...\n");
    
    MockX11 mock = setup_mock_x11();
    MenuManager* manager = menu_manager_create(mock.conn, &mock.ewmh);
    
    /* Create test menu */
    Menu* menu = create_test_menu(manager->focus_ctx, "Test Menu",
                                 XCB_MOD_MASK_4, 31);  // Super+i
    menu_manager_register(manager, menu);
    
    /* Test activation */
    assert(menu_manager_activate(manager, menu));
    assert(manager->active_menu == menu);
    assert(menu_manager_get_active(manager) == menu);
    
    /* Test deactivation */
    menu_manager_deactivate(manager);
    assert(manager->active_menu == NULL);
    assert(menu_manager_get_active(manager) == NULL);
    
    /* Clean up */
    menu_manager_destroy(manager);
    cleanup_mock_x11(&mock);
    
    printf("Menu activation test passed\n");
}

/* Test input handling */
static void test_input_handling(void) {
    printf("Testing input handling...\n");
    
    MockX11 mock = setup_mock_x11();
    MenuManager* manager = menu_manager_create(mock.conn, &mock.ewmh);
    
    /* Create test menu */
    Menu* menu = create_test_menu(manager->focus_ctx, "Test Menu",
                                 XCB_MOD_MASK_4, 31);  // Super+i
    menu_manager_register(manager, menu);
    
    /* Simulate key press events */
    xcb_key_press_event_t press = {
        .response_type = XCB_KEY_PRESS,
        .detail = 31,        // 'i' key
        .state = XCB_MOD_MASK_4  // Super key
    };
    
    /* Test menu activation via key press */
    assert(menu_manager_handle_key_press(manager, &press));
    assert(manager->active_menu == menu);
    
    /* Test navigation */
    press.detail = 44;  // 'j' key
    assert(menu_manager_handle_key_press(manager, &press));
    
    /* Test deactivation via escape */
    press.detail = 9;  // escape key
    press.state = 0;
    assert(!menu_manager_handle_key_press(manager, &press));
    assert(manager->active_menu == NULL);
    
    /* Clean up */
    menu_manager_destroy(manager);
    cleanup_mock_x11(&mock);
    
    printf("Input handling test passed\n");
}

/* Run all tests */
int main(void) {
    printf("Running menu manager tests...\n\n");
    
    test_manager_lifecycle();
    test_menu_registration();
    test_menu_activation();
    test_input_handling();
    
    printf("\nAll menu manager tests passed!\n");
    return 0;
}