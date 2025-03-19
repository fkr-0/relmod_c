/* clock_menu.c - Example dynamic clock menu implementation */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* Clock menu data */
typedef struct {
    char** time_labels;     /* Array of time format strings */
    size_t label_count;     /* Number of time formats */
    struct timeval last_update;  /* Last update timestamp */
} ClockData;

/* Available time formats */
static const char* TIME_FORMATS[] = {
    "%H:%M:%S",           /* 24-hour with seconds */
    "%I:%M:%S %p",        /* 12-hour with seconds */
    "%Y-%m-%d %H:%M:%S",  /* Full date and time */
    "%A, %B %d",          /* Day and date */
    "%Z %z"               /* Timezone */
};

#define FORMAT_COUNT (sizeof(TIME_FORMATS) / sizeof(TIME_FORMATS[0]))
#define MAX_TIME_LEN 64

/* Format current time according to format string */
static void format_time(const char* format, char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, format, tm_info);
}

/* Update time labels */
static void update_clock_labels(ClockData* data) {
    for (size_t i = 0; i < data->label_count; i++) {
        format_time(TIME_FORMATS[i], data->time_labels[i], MAX_TIME_LEN);
    }
}

/* Initialize clock data */
static ClockData* clock_data_create(void) {
    ClockData* data = calloc(1, sizeof(ClockData));
    if (!data) return NULL;
    
    data->label_count = FORMAT_COUNT;
    data->time_labels = calloc(data->label_count, sizeof(char*));
    
    if (!data->time_labels) {
        free(data);
        return NULL;
    }
    
    /* Allocate and initialize time labels */
    for (size_t i = 0; i < data->label_count; i++) {
        data->time_labels[i] = calloc(MAX_TIME_LEN, sizeof(char));
        if (!data->time_labels[i]) {
            for (size_t j = 0; j < i; j++) {
                free(data->time_labels[j]);
            }
            free(data->time_labels);
            free(data);
            return NULL;
        }
    }
    
    /* Initial update */
    update_clock_labels(data);
    gettimeofday(&data->last_update, NULL);
    
    return data;
}

/* Clean up clock data */
static void clock_data_destroy(ClockData* data) {
    if (!data) return;
    
    if (data->time_labels) {
        for (size_t i = 0; i < data->label_count; i++) {
            free(data->time_labels[i]);
        }
        free(data->time_labels);
    }
    
    free(data);
}

/* Menu action callback */
static void clock_action(void* user_data) {
    /* No action needed for clock display */
    (void)user_data;
}

/* Menu update callback */
static void clock_update(Menu* menu, void* user_data) {
    if (!menu || !user_data) return;
    ClockData* data = (ClockData*)user_data;
    
    /* Update time labels */
    update_clock_labels(data);
    
    /* Update menu items */
    for (size_t i = 0; i < data->label_count; i++) {
        menu->config.items[i].label = data->time_labels[i];
    }
}

/* Create clock menu */
Menu* create_clock_menu(xcb_connection_t* conn, xcb_window_t root) {
    /* Create and initialize clock data */
    ClockData* data = clock_data_create();
    if (!data) return NULL;
    
    /* Create menu items */
    MenuItem* items = calloc(FORMAT_COUNT, sizeof(MenuItem));
    if (!items) {
        clock_data_destroy(data);
        return NULL;
    }
    
    /* Initialize menu items */
    for (size_t i = 0; i < FORMAT_COUNT; i++) {
        items[i] = (MenuItem){
            .id = TIME_FORMATS[i],
            .label = data->time_labels[i],
            .action = clock_action,
            .metadata = NULL
        };
    }
    
    /* Configure menu */
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,    /* Super key */
        .trigger_key = 54,             /* 'c' key */
        .title = "Clock Menu",
        .items = items,
        .item_count = FORMAT_COUNT,
        .nav = {
            .next = { .key = 44, .label = "j" },
            .prev = { .key = 45, .label = "k" }
        },
        .act = {
            .activate_on_mod_release = false,
            .activate_on_direct_key = false
        },
        .style = {
            .background_color = {0.1, 0.1, 0.1, 0.9},
            .text_color = {0.8, 0.8, 0.8, 1.0},
            .highlight_color = {0.3, 0.3, 0.8, 1.0},
            .font_face = "Monospace",  /* Fixed-width font for alignment */
            .font_size = 14.0,
            .item_height = 20,
            .padding = 10
        }
    };
    
    /* Create menu */
    Menu* menu = cairo_menu_create(conn, root, &config);
    free(items);  /* Menu creates its own copy */
    
    if (!menu) {
        clock_data_destroy(data);
        return NULL;
    }
    
    /* Set up menu callbacks */
    menu->update_cb = clock_update;
    menu->user_data = data;
    
    /* Configure update interval (1 second) */
    menu_set_update_interval(menu, 1000);
    
    return menu;
}

/* Example usage */
int main(void) {
    xcb_connection_t* conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to X server\n");
        return 1;
    }
    
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    Menu* menu = create_clock_menu(conn, screen->root);
    
    if (!menu) {
        fprintf(stderr, "Failed to create clock menu\n");
        xcb_disconnect(conn);
        return 1;
    }
    
    printf("Clock menu created successfully\n");
    printf("Press Super+C to show clock\n");
    
    // Menu would be registered with menu manager here
    // menu_manager_register(manager, menu);
    
    // Cleanup for test
    menu_destroy(menu);
    xcb_disconnect(conn);
    
    return 0;
}