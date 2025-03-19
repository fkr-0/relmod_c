/* plugin_test_template.c - Test template for menu plugins */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/* Mock X11 environment for testing */
typedef struct {
    xcb_connection_t* conn;
    xcb_window_t root;
    xcb_screen_t* screen;
} MockX11;

/* Test event simulation */
typedef struct {
    uint8_t keycode;
    uint16_t state;
    bool is_press;
} TestKeyEvent;

/* Performance measurement */
typedef struct {
    struct timeval start;
    struct timeval end;
} Timer;

/* Initialize mock X11 environment */
static MockX11 setup_mock_x11(void) {
    MockX11 mock = {0};
    mock.conn = xcb_connect(NULL, NULL);
    assert(!xcb_connection_has_error(mock.conn));
    
    mock.screen = xcb_setup_roots_iterator(xcb_get_setup(mock.conn)).data;
    mock.root = mock.screen->root;
    
    return mock;
}

/* Clean up mock environment */
static void cleanup_mock_x11(MockX11* mock) {
    if (mock->conn) {
        xcb_disconnect(mock->conn);
    }
}

/* Timer utilities */
static void timer_start(Timer* timer) {
    gettimeofday(&timer->start, NULL);
}

static double timer_end(Timer* timer) {
    gettimeofday(&timer->end, NULL);
    return (timer->end.tv_sec - timer->start.tv_sec) * 1000.0 +
           (timer->end.tv_usec - timer->start.tv_usec) / 1000.0;
}

/* Simulate key event */
static void simulate_key_event(Menu* menu, TestKeyEvent event) {
    if (event.is_press) {
        xcb_key_press_event_t press = {
            .response_type = XCB_KEY_PRESS,
            .detail = event.keycode,
            .state = event.state
        };
        menu_handle_key_press(menu, &press);
    } else {
        xcb_key_release_event_t release = {
            .response_type = XCB_KEY_RELEASE,
            .detail = event.keycode,
            .state = event.state
        };
        menu_handle_key_release(menu, &release);
    }
}

/* Test creation and destruction */
static void test_plugin_lifecycle(void) {
    printf("Testing plugin lifecycle...\n");
    
    MockX11 mock = setup_mock_x11();
    Timer timer;
    
    /* Test creation time */
    timer_start(&timer);
    Menu* menu = create_plugin_menu(mock.conn, mock.root);
    double create_time = timer_end(&timer);
    
    assert(menu != NULL);
    assert(menu->user_data != NULL);
    printf("Creation time: %.3f ms\n", create_time);
    
    /* Test destruction time */
    timer_start(&timer);
    menu_destroy(menu);
    double destroy_time = timer_end(&timer);
    printf("Destruction time: %.3f ms\n", destroy_time);
    
    cleanup_mock_x11(&mock);
    printf("Lifecycle test passed\n\n");
}

/* Test menu items */
static void test_plugin_items(void) {
    printf("Testing plugin items...\n");
    
    MockX11 mock = setup_mock_x11();
    Menu* menu = create_plugin_menu(mock.conn, mock.root);
    assert(menu != NULL);
    
    /* Verify items */
    assert(menu->config.items != NULL);
    assert(menu->config.item_count > 0);
    
    /* Check item properties */
    for (size_t i = 0; i < menu->config.item_count; i++) {
        MenuItem* item = &menu->config.items[i];
        assert(item->id != NULL);
        assert(item->label != NULL);
        assert(item->action != NULL);
    }
    
    menu_destroy(menu);
    cleanup_mock_x11(&mock);
    printf("Items test passed\n\n");
}

/* Test input handling */
static void test_plugin_input(void) {
    printf("Testing plugin input handling...\n");
    
    MockX11 mock = setup_mock_x11();
    Menu* menu = create_plugin_menu(mock.conn, mock.root);
    assert(menu != NULL);
    
    /* Test navigation */
    TestKeyEvent events[] = {
        { .keycode = 44, .state = 0, .is_press = true },   // Next
        { .keycode = 45, .state = 0, .is_press = true },   // Prev
        { .keycode = 10, .state = 0, .is_press = true }    // Direct
    };
    
    for (size_t i = 0; i < sizeof(events)/sizeof(events[0]); i++) {
        simulate_key_event(menu, events[i]);
    }
    
    menu_destroy(menu);
    cleanup_mock_x11(&mock);
    printf("Input test passed\n\n");
}

/* Test updates (if dynamic) */
static void test_plugin_updates(void) {
    printf("Testing plugin updates...\n");
    
    MockX11 mock = setup_mock_x11();
    Menu* menu = create_plugin_menu(mock.conn, mock.root);
    assert(menu != NULL);
    
    /* Get initial state */
    char* initial_label = strdup(menu->config.items[0].label);
    
    /* Simulate time passing */
    usleep(100000);  // 100ms
    
    /* Force update if plugin supports it */
    if (menu->update_cb) {
        menu->update_cb(menu, menu->user_data);
        
        /* Check if content changed */
        if (strcmp(initial_label, menu->config.items[0].label) != 0) {
            printf("Content updated successfully\n");
        }
    }
    
    free(initial_label);
    menu_destroy(menu);
    cleanup_mock_x11(&mock);
    printf("Update test passed\n\n");
}

/* Performance tests */
static void test_plugin_performance(void) {
    printf("Testing plugin performance...\n");
    
    MockX11 mock = setup_mock_x11();
    Timer timer;
    double total_time = 0;
    const int ITERATIONS = 100;
    
    /* Measure average creation time */
    for (int i = 0; i < ITERATIONS; i++) {
        timer_start(&timer);
        Menu* menu = create_plugin_menu(mock.conn, mock.root);
        total_time += timer_end(&timer);
        menu_destroy(menu);
    }
    
    printf("Average creation time: %.3f ms\n", total_time / ITERATIONS);
    
    /* Test memory usage */
    Menu* menu = create_plugin_menu(mock.conn, mock.root);
    
    /* Simulate intensive usage */
    for (int i = 0; i < 1000; i++) {
        TestKeyEvent event = {
            .keycode = 44,
            .state = 0,
            .is_press = true
        };
        simulate_key_event(menu, event);
        
        if (menu->update_cb) {
            menu->update_cb(menu, menu->user_data);
        }
    }
    
    menu_destroy(menu);
    cleanup_mock_x11(&mock);
    printf("Performance test passed\n\n");
}

/* Run all tests */
int main(void) {
    printf("Running Plugin Tests\n");
    printf("===================\n\n");
    
    test_plugin_lifecycle();
    test_plugin_items();
    test_plugin_input();
    test_plugin_updates();
    test_plugin_performance();
    
    printf("All plugin tests passed!\n");
    return 0;
}