/* clock_menu.c - Example dynamic clock menu implementation */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
#include "../src/menu_manager.h"
#include "../src/input_handler.h"
#include <stdbool.h>
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
static void clock_data_destroy(void* user_data) {
    ClockData* data = (ClockData*)user_data;
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
// Creates the clock menu configuration and menu structure.
// The caller is responsible for registering it with a MenuManager/InputHandler
// and setting up rendering.
Menu* create_clock_menu(void) {
    // Create and initialize clock data
    ClockData* data = clock_data_create();
    if (!data) return NULL;

    // Create menu items dynamically based on clock data
    MenuItem* items = calloc(FORMAT_COUNT, sizeof(MenuItem));
    if (!items) {
        clock_data_destroy(data);
        return NULL;
    }

    // Initialize menu items using the initial time labels
    for (size_t i = 0; i < FORMAT_COUNT; i++) {
        items[i] = (MenuItem){
            .id = strdup(TIME_FORMATS[i]), // Use format string as ID, needs freeing
            .label = strdup(data->time_labels[i]), // Copy initial label, needs freeing
            .action = clock_action,
            .metadata = NULL // Action doesn't need metadata
        };
        // Check allocation success
        if (!items[i].id || !items[i].label) {
             for(size_t j = 0; j <= i; ++j) { free((void*)items[j].id); free((void*)items[j].label); }
             free(items);
             clock_data_destroy(data);
             return NULL;
        }
    }

    // Configure menu
    // Note: The items array passed here will be copied by menu_create.
    // We need to free our allocated items array afterwards.
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,    // Super key
        .trigger_key = 54,             // 'c' key
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
        .style = { // Example style
            .background_color = {0.1, 0.1, 0.1, 0.9},
            .text_color = {0.8, 0.8, 0.8, 1.0},
            .highlight_color = {0.3, 0.3, 0.8, 1.0},
            .font_face = "Monospace",
            .font_size = 14.0,
            .item_height = 20,
            .padding = 10
        }
    };

    // Create the menu using the configuration
    Menu* menu = menu_create(&config);

    // Free the temporary items array we created (including allocated id/label)
    // menu_create makes its own copies.
    for (size_t i = 0; i < FORMAT_COUNT; i++) {
        free((void*)items[i].id);
        free((void*)items[i].label);
    }
    free(items);

    if (!menu) {
        clock_data_destroy(data);
        return NULL;
    }

    // Set up menu callbacks and user data
    menu->update_cb = clock_update;
    menu->user_data = data; // Transfer ownership of data to the menu
    menu->cleanup_cb = clock_data_destroy; // Set cleanup callback for user_data

    // Configure update interval (1 second)
    menu_set_update_interval(menu, 1000);

    return menu;
}

/* Example usage */
// Example main function demonstrating how to use the clock menu
int main(void) {
    // 1. Create the Input Handler and set up X connection
    InputHandler *handler = input_handler_create();
    if (!handler) {
        fprintf(stderr, "Failed to create input handler\n");
        return 1;
    }
    if (!input_handler_setup_x(handler)) {
         fprintf(stderr, "Failed to setup X for input handler\n");
         input_handler_destroy(handler); // Cleanup handler
         return 1;
    }

    // 2. Create the clock menu
    Menu* clock_menu = create_clock_menu();
    if (!clock_menu) {
        fprintf(stderr, "Failed to create clock menu\n");
        input_handler_destroy(handler); // Cleanup handler
        return 1;
    }

    // 3. Register the menu with the Input Handler's Menu Manager
    input_handler_add_menu(handler, clock_menu); // Transfers ownership if successful

    printf("Clock menu created and registered.\n");
    printf("Press Super+C to activate.\n");
    printf("Press ESC or q to exit.\n");

    // 4. Run the input handler's event loop
    input_handler_run(handler); // This will block until exit

    // 5. Cleanup (Input Handler destroy will also destroy registered menus)
    printf("Exiting...\n");
    input_handler_destroy(handler);

    return 0;
}
