/* test_integration.c - Integration tests for menu system */
#include "../src/cairo_menu.h"
#include "../src/input_handler.h"
#include "../src/key_helper.h"
#include "../src/menu_builder.h"
#include "../src/menu_manager.h"
#include <X11/keysym.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void demo_action(void *user_data) {
    printf("Menu item selected: %s\n", (char *)user_data);
}
/* Mock X11 environment */
typedef struct {
    xcb_connection_t *conn;
    xcb_window_t root;
    xcb_screen_t *screen;
    xcb_ewmh_connection_t ewmh;
} MockX11;

/* Test action callback counter */
static int action_count = 0;

/* Mock action callback */
static void test_action(void *user_data) {
    action_count++;
    const char *item_id = (const char *)user_data;
    printf("Action triggered for item: %s\n", item_id);
}

/* Initialize mock X11 environment */
static MockX11 setup_mock_x11(void) {
    MockX11 mock = {0};
    // Connection logic (including retries) is now handled by input_handler_setup_x
    // We still need a basic connection here for EWMH setup, but rely on the handler setup for robustness.
    mock.conn = xcb_connect(NULL, NULL);
    assert(mock.conn != NULL && !xcb_connection_has_error(mock.conn) && "Initial mock connection failed");

    mock.screen = xcb_setup_roots_iterator(xcb_get_setup(mock.conn)).data;
    mock.root = mock.screen->root;

    xcb_intern_atom_cookie_t *cookie =
        xcb_ewmh_init_atoms(mock.conn, &mock.ewmh);
    assert(xcb_ewmh_init_atoms_replies(&mock.ewmh, cookie, NULL));

    return mock;
}

/* Clean up mock environment */
static void cleanup_mock_x11(MockX11 *mock) {
    xcb_ewmh_connection_wipe(&mock->ewmh);
    xcb_disconnect(mock->conn);
}

/* Simulate key press event */
static int simulate_key_press(InputHandler *handler, uint8_t keycode,
                              uint16_t state) {
    xcb_key_press_event_t event = {.response_type = XCB_KEY_PRESS,
                                   .detail = keycode,
                                   .state = state,
                                   .time = XCB_CURRENT_TIME};
    int new_state = state;
    if (keycode == 64) {
        new_state |= 0x08;
    } else if (keycode == 37) {
        new_state |= 0x04;
    } else if (keycode == 50) {
        new_state |= 0x01;
    } else if (keycode == 133) {
        new_state |= 0x40;
    }

    printf("SimulatedPress: %d New state: 0x%x\n", keycode, new_state);
    xcb_generic_event_t *generic = (xcb_generic_event_t *)&event;
    bool exit = input_handler_handle_event(
        handler, generic); // input_handler_process_event(handler);

    // assert(!exit); /* Shouldn't exit during normal operation */
    return new_state;
}

/* Simulate key release event */
static int simulate_key_release(InputHandler *handler, uint8_t keycode,
                                uint16_t state) {
    xcb_key_release_event_t event = {.response_type = XCB_KEY_RELEASE,
                                     .detail = keycode,
                                     .state = state,
                                     .time = XCB_CURRENT_TIME};

    int new_state = state;
    if (keycode == 64) {
        new_state &= ~0x08;
    } else if (keycode == 37) {
        new_state &= ~0x04;
    } else if (keycode == 50) {
        new_state &= ~0x01;
    } else if (keycode == 133) {
        new_state &= ~0x40;
    }
    printf("SimulatedRelease: %d New state: 0x%x\n", keycode, new_state);
    xcb_generic_event_t *generic = (xcb_generic_event_t *)&event;
    bool exit = // input_handler_process_event(handler);
        input_handler_handle_event(
            handler, generic); // input_handler_process_event(handler);
    /* assert(exit == (new_state == 0)); /\* Should exit when modifier is
     * released *\/ */
    return new_state;
}


/* Process pending X events to allow state updates */
static void process_pending_events(InputHandler *handler) {
    // Allow some time for events to potentially arrive and be processed
    // Adjust sleep duration if needed, but keep it short for tests.
    usleep(50000); // 50ms

    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(handler->conn))) {
        printf("Processing pending event: type %d\n", event->response_type & ~0x80);
        input_handler_handle_event(handler, event);
        free(event);
    }
    // Ensure requests sent during event handling are flushed
    xcb_flush(handler->conn);
    // Another short sleep might be needed after processing
    usleep(50000); // 50ms
}

/* Test complete menu workflow */
static void test_menu_workflow(void) {
    printf("Testing complete menu workflow...\n");

    InputHandler *handler = input_handler_create();
    assert(handler != NULL && "Failed to create input handler");
    assert(input_handler_setup_x(handler) && "Input handler X setup failed");

    /* static MenuItem items[] = {{.id = "item1", */
    /*                             .label = "Item 1", */
    /*                             .action = test_action, */
    /*                             .metadata = "item1"}, */
    /*                            {.id = "item2", */
    /*                             .label = "Item 2", */
    /*                             .action = test_action, */
    /*                             .metadata = "item2"}, */
    /*                            {.id = "item3", */
    /*                             .label = "Item 3", */
    /*                             .action = test_action, */
    /*                             .metadata = "item3"}}; */
    /* NavigationConfig nav = {.next = {.key = 44, .label = "j"}, */
    /*                         .prev = {.key = 45, .label = "k"}, */
    /*                         .direct = {.keys = NULL, .count = 0}, */
    /*                         .extension_data = NULL}; */

    MenuBuilder builder1 = menu_builder_create("Menu 1", 3);
    menu_builder_add_item(&builder1, "Item 1", test_action, "item1");
    menu_builder_add_item(&builder1, "Item 2", test_action, "item2");
    menu_builder_add_item(&builder1, "Item 3", test_action, "item3");
    menu_builder_set_mod_key(&builder1, XCB_MOD_MASK_4); // Super key
    menu_builder_set_trigger_key(&builder1, 31);         // Space key
    menu_builder_set_activation_state(&builder1, XCB_MOD_MASK_4,
                                      31); // Super + i
    menu_builder_set_navigation_keys(&builder1, 44, "j", 45, "k", NULL,
                                     0); // j, k
    MenuConfig *config1 = menu_builder_finalize(&builder1);
    printf("Menu config created: %p\n", config1);
    Menu *menu1 = menu_create(config1);
    assert(input_handler_add_menu(handler, menu1));
    printf("Menu created: %p\n", menu1);

    int state = 0;
    printf("1. Activate menu (Super+i)\n");
    state = simulate_key_press(handler, SUPER_KEY, state); /* Super down */
    state = simulate_key_press(handler, 31, state);        /* i press */
    process_pending_events(handler); // Process events after activation
    printf("Active menu: %s\n", menu1->config.title);
    printf("Active menu: %s\n",
           menu_manager_get_active(handler->menu_manager)->config.title);
    assert(state == SUPER_MASK);
    assert(menu_manager_get_active(handler->menu_manager) == menu1);

    printf("2. Navigate down\n");
    /* state = simulate_key_press(handler, 44, state);   /\* j press *\/ */
    /* state = simulate_key_release(handler, 44, state); /\* j press *\/ */
    state = simulate_key_press(handler, 44, state);   /* j press */
    state = simulate_key_release(handler, 44, state); /* j press */
    process_pending_events(handler); // Process events after navigation
    printf("Selected index: %d\n", menu1->selected_index);
    assert(menu1->selected_index == 1);

    printf("3. Navigate up\n");
    state = simulate_key_press(handler, 45, state); /* k press */
    process_pending_events(handler); // Process events after navigation
    assert(menu1->selected_index == 0);

    printf("4. Direct selection\n");
    state = simulate_key_press(handler, 11, state); /* 2 press */
    process_pending_events(handler); // Process events after navigation
    assert(menu1->selected_index == 1);

    printf("5. Trigger action and deactivate\n");
    action_count = 0;
    state = simulate_key_release(handler, SUPER_KEY, state); /* Super up */
    process_pending_events(handler); // Process events after deactivation
    assert(action_count == 1);
    assert(menu_manager_get_active(handler->menu_manager) == NULL);

    input_handler_destroy(handler);
    printf("Menu workflow test passed\n");
}

/* Test menu switching */
static void test_menu_switching(void) {
    printf("Testing menu switching...\n");

    printf("Testing complete menu workflow...\n");

    /* MockX11 mock = setup_mock_x11(); */

    /* Create input handler */
    /* InputHandler *handler = input_handler_create(); */
    /* handler->conn = mock.conn; */
    /* handler->screen = mock.screen; */
    /* handler->root = &mock.root; */
    /* handler->ewmh = &mock.ewmh; */

    InputHandler *handler = input_handler_create();
    assert(handler != NULL && "Failed to create input handler");
    assert(input_handler_setup_x(handler) && "Input handler X setup failed");

    /* Create two test menus */
    /* static MenuItem items1[] = { */
    /*     {.id = "menu1_item1", .label = "Menu 1 Item 1", .action =
     * test_action}}; */
    /* static MenuItem items2[] = { */
    /*     {.id = "menu2_item1", .label = "Menu 2 Item 1", .action =
     * test_action}}; */

    /* MenuConfig config1 = {.mod_key = XCB_MOD_MASK_4, */
    /*                       .trigger_key = 31, /\* i *\/ */
    /*                       .title = "Menu 1", */
    /*                       .items = items1, */
    /*                       .item_count = 1}; */

    /* MenuConfig config2 = {.mod_key = XCB_MOD_MASK_4, */
    /*                       .trigger_key = 32, /\* o *\/ */
    /*                       .title = "Menu 2", */
    /*                       .items = items2, */
    /*                       .item_count = 1}; */

    MenuBuilder builder1 = menu_builder_create("Menu 1", 1);
    menu_builder_add_item(&builder1, "Item 1", demo_action, "item1");
    menu_builder_set_mod_key(&builder1, XCB_MOD_MASK_4); // Super key
    menu_builder_set_trigger_key(&builder1, 31);         // Space key
    menu_builder_set_activation_state(&builder1, XCB_MOD_MASK_4,
                                      31); // Super + i
    MenuConfig *config1 = menu_builder_finalize(&builder1);

    MenuBuilder builder2 = menu_builder_create("Menu 2", 1);
    menu_builder_add_item(&builder2, "Item 1", demo_action, "item1");
    menu_builder_set_mod_key(&builder2, XCB_MOD_MASK_4); // Super key
    menu_builder_set_trigger_key(&builder2, 32);         // Space key
    menu_builder_set_activation_state(&builder2, XCB_MOD_MASK_4,
                                      32); // Super + o
    MenuConfig *config2 = menu_builder_finalize(&builder2);

    Menu *menu1 = menu_create(config1);
    Menu *menu2 = menu_create(config2);
    assert(input_handler_add_menu(handler, menu1));
    assert(input_handler_add_menu(handler, menu2));

    /* Test switching between menus */

    int state = 0;
    printf("1. Activate menu (Super+i)\n");
    state = simulate_key_press(handler, SUPER_KEY, state); /* Super down */
    state = simulate_key_press(handler, 31, state);        /* i press */
    process_pending_events(handler); // Process events after activation
    printf("Active menu: %s\n", menu1->config.title);
    printf("Active menu: %s\n",
           menu_manager_get_active(handler->menu_manager)->config.title);
    assert(state == SUPER_MASK);
    assert(menu_manager_get_active(handler->menu_manager) == menu1);

    /* Switch to second menu */
    /* state = simulate_key_release(handler, 31, XCB_MOD_MASK_4); /\* i release
     * *\/ */
    printf("2. Activate menu (Super+o)\n");
    state = simulate_key_press(handler, 32, XCB_MOD_MASK_4);   /* o press */
    state = simulate_key_release(handler, 32, XCB_MOD_MASK_4); /* o press */
    process_pending_events(handler); // Process events after switch attempt
    assert(menu_manager_get_active(handler->menu_manager) == menu2);

    /* fprintf(stderr, "Active menu: %s\n", */
    /*         menu_manager_get_active(handler->menu_manager)); */
    /* Deactivate */
    printf("3. Deactivate menu (Super up)\n");
    state = simulate_key_release(handler, SUPER_KEY, state); /* Super up */
    process_pending_events(handler); // Process events after deactivation
    assert(menu_manager_get_active(handler->menu_manager) == NULL);

    /* Clean up */
    input_handler_destroy(handler);
    /* cleanup_mock_x11(&mock); */

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
