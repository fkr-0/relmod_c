/* test_integration.c - Integration tests for menu system */
#include "../src/cairo_menu.h"
#include "../src/input_handler.h"
#include "../src/menu_manager.h"
#include "../src/window_menu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/keysym.h>

/* Mock X11 environment */
typedef struct {
    xcb_connection_t* conn;
    xcb_window_t root;
    xcb_screen_t* screen;
    xcb_ewmh_connection_t ewmh;
} MockX11;

/* Test action callback counter */
static int action_count = 0;

/* Mock action callback */
static void test_action(void* user_data) {
    action_count++;
    const char* item_id = (const char*)user_data;
    printf("Action triggered for item: %s\n", item_id);
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

/* Simulate key press event */
static void simulate_key_press(InputHandler* handler, uint8_t keycode, uint16_t state) {
    xcb_key_press_event_t event = {
        .response_type = XCB_KEY_PRESS,
        .detail = keycode,
        .state = state,
        .time = XCB_CURRENT_TIME
    };
    
    xcb_generic_event_t* generic = (xcb_generic_event_t*)&event;
    bool exit = input_handler_process_event(handler);
    assert(!exit);  /* Shouldn't exit during normal operation */
}

/* Simulate key release event */
static void simulate_key_release(InputHandler* handler, uint8_t keycode, uint16_t state) {
    xcb_key_release_event_t event = {
        .response_type = XCB_KEY_RELEASE,
        .detail = keycode,
        .state = state,
        .time = XCB_CURRENT_TIME
    };
    
    xcb_generic_event_t* generic = (xcb_generic_event_t*)&event;
    bool exit = input_handler_process_event(handler);
    assert(!exit);
}

/* Test complete menu workflow */
static void test_menu_workflow(void) {
    printf("Testing complete menu workflow...\n");
    
    MockX11 mock = setup_mock_x11();
    
    /* Create input handler */
    InputHandler* handler = input_handler_create(mock.conn, &mock.ewmh, mock.root);
    assert(handler != NULL);
    
    /* Create and register test menu */
    static MenuItem items[] = {
        { .id = "item1", .label = "Item 1", .action = test_action, .metadata = "item1" },
        { .id = "item2", .label = "Item 2", .action = test_action, .metadata = "item2" },
        { .id = "item3", .label = "Item 3", .action = test_action, .metadata = "item3" }
    };
    
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,    /* Super key */
        .trigger_key = 31,             /* 'i' key */
        .title = "Test Menu",
        .items = items,
        .item_count = 3,
        .nav = {
            .next = { .key = 44, .label = "j" },    /* j key */
            .prev = { .key = 45, .label = "k" },    /* k key */
            .direct = {
                .keys = (uint8_t[]){10, 11, 12},    /* 1-3 keys */
                .count = 3
            }
        },
        .act = {
            .activate_on_mod_release = true,
            .activate_on_direct_key = true
        }
    };
    
    Menu* menu = cairo_menu_create(mock.conn, mock.root, &config);
    assert(menu != NULL);
    assert(menu_manager_register(handler->menu_manager, menu));
    
    /* Test workflow: Activate -> Navigate -> Select -> Deactivate */
    
    /* 1. Activate menu (Super+i) */
    simulate_key_press(handler, XCB_MOD_MASK_4, 0);    /* Super down */
    simulate_key_press(handler, 31, XCB_MOD_MASK_4);   /* i press */
    assert(menu_manager_get_active(handler->menu_manager) == menu);
    
    /* 2. Navigate down */
    simulate_key_press(handler, 44, XCB_MOD_MASK_4);   /* j press */
    assert(menu->selected_index == 1);
    
    /* 3. Navigate up */
    simulate_key_press(handler, 45, XCB_MOD_MASK_4);   /* k press */
    assert(menu->selected_index == 0);
    
    /* 4. Direct selection */
    simulate_key_press(handler, 11, XCB_MOD_MASK_4);   /* 2 press */
    assert(menu->selected_index == 1);
    
    /* 5. Trigger action and deactivate */
    action_count = 0;
    simulate_key_release(handler, XCB_MOD_MASK_4, 0);  /* Super up */
    assert(action_count == 1);
    assert(menu_manager_get_active(handler->menu_manager) == NULL);
    
    /* Clean up */
    input_handler_destroy(handler);
    cleanup_mock_x11(&mock);
    
    printf("Menu workflow test passed\n");
}

/* Test menu switching */
static void test_menu_switching(void) {
    printf("Testing menu switching...\n");
    
    MockX11 mock = setup_mock_x11();
    InputHandler* handler = input_handler_create(mock.conn, &mock.ewmh, mock.root);
    assert(handler != NULL);
    
    /* Create two test menus */
    static MenuItem items1[] = {
        { .id = "menu1_item1", .label = "Menu 1 Item 1", .action = test_action }
    };
    static MenuItem items2[] = {
        { .id = "menu2_item1", .label = "Menu 2 Item 1", .action = test_action }
    };
    
    MenuConfig config1 = {
        .mod_key = XCB_MOD_MASK_4,
        .trigger_key = 31,  /* i */
        .title = "Menu 1",
        .items = items1,
        .item_count = 1
    };
    
    MenuConfig config2 = {
        .mod_key = XCB_MOD_MASK_4,
        .trigger_key = 32,  /* o */
        .title = "Menu 2",
        .items = items2,
        .item_count = 1
    };
    
    Menu* menu1 = cairo_menu_create(mock.conn, mock.root, &config1);
    Menu* menu2 = cairo_menu_create(mock.conn, mock.root, &config2);
    
    assert(menu_manager_register(handler->menu_manager, menu1));
    assert(menu_manager_register(handler->menu_manager, menu2));
    
    /* Test switching between menus */
    
    /* Activate first menu */
    simulate_key_press(handler, XCB_MOD_MASK_4, 0);    /* Super down */
    simulate_key_press(handler, 31, XCB_MOD_MASK_4);   /* i press */
    assert(menu_manager_get_active(handler->menu_manager) == menu1);
    
    /* Switch to second menu */
    simulate_key_release(handler, 31, XCB_MOD_MASK_4); /* i release */
    simulate_key_press(handler, 32, XCB_MOD_MASK_4);   /* o press */
    assert(menu_manager_get_active(handler->menu_manager) == menu2);
    
    /* Deactivate */
    simulate_key_release(handler, XCB_MOD_MASK_4, 0);  /* Super up */
    assert(menu_manager_get_active(handler->menu_manager) == NULL);
    
    /* Clean up */
    input_handler_destroy(handler);
    cleanup_mock_x11(&mock);
    
    printf("Menu switching test passed\n");
}

/* Run all integration tests */
int main(void) {
    printf("Running integration tests...\n\n");
    
    test_menu_workflow();
    test_menu_switching();
    
    printf("\nAll integration tests passed!\n");
    return 0;
}