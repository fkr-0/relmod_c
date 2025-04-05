/* system_monitor_menu.c - Example system monitor menu implementation */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
#include "../src/menu_manager.h"
#include "../src/input_handler.h"
#include <stdbool.h> // For bool type
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

/* System information data */
typedef struct {
    float cpu_usage;
    float mem_usage;
    unsigned int proc_count;
    char* top_process;
} SystemInfo;

/* Menu state */
typedef struct {
    SystemInfo info;
    // Items are now managed directly by the Menu struct after creation
    size_t item_count;
    unsigned int update_interval; // ms
} SystemMenuData;

/* Read CPU usage from /proc/stat */
static float get_cpu_usage(void) {
    static long prev_idle = 0, prev_total = 0;
    long idle = 0, total = 0;
    float usage = 0.0;

    FILE* fp = fopen("/proc/stat", "r");
    if (fp) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            long user, nice, system, idle_time, iowait, irq, softirq;
            sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld",
                   &user, &nice, &system, &idle_time, &iowait, &irq, &softirq);

            idle = idle_time + iowait;
            total = user + nice + system + idle + irq + softirq;

            if (prev_total != 0) {
                long diff_idle = idle - prev_idle;
                long diff_total = total - prev_total;
                usage = (1.0 - (float)diff_idle / diff_total) * 100.0;
            }

            prev_idle = idle;
            prev_total = total;
        }
        fclose(fp);
    }

    return usage;
}

/* Get memory usage from sysinfo */
static float get_memory_usage(void) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        unsigned long total = si.totalram;
        unsigned long free = si.freeram;
        return ((float)(total - free) / total) * 100.0;
    }
    return 0.0;
}

/* Get number of running processes */
static unsigned int get_process_count(void) {
    DIR* dir = opendir("/proc");
    unsigned int count = 0;

    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            struct stat st;
            char path[256];
            snprintf(path, sizeof(path), "/proc/%s", entry->d_name);

            if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char* endptr;
                strtol(entry->d_name, &endptr, 10);
                if (*endptr == '\0') count++;
            }
        }
        closedir(dir);
    }

    return count;
}

/* Get top CPU using process */
static char* get_top_process(void) {
    static char name[256];
    FILE* fp = popen("ps -aux --sort=-pcpu | head -n 2 | tail -n 1 | awk '{print $11}'", "r");

    if (fp) {
        if (fgets(name, sizeof(name), fp)) {
            name[strcspn(name, "\n")] = 0;
        } else {
            strcpy(name, "Unknown");
        }
        pclose(fp);
    } else {
        strcpy(name, "Unknown");
    }

    return name;
}

/* Update system information */
static void update_system_info(SystemInfo* info) {
    info->cpu_usage = get_cpu_usage();
    info->mem_usage = get_memory_usage();
    info->proc_count = get_process_count();
    info->top_process = get_top_process();
}

/* Update menu items with current system info */
// Updates the labels of the menu items based on current system info.
// Assumes menu->user_data points to a valid SystemMenuData.
static void update_menu_items(Menu* menu) {
    if (!menu || !menu->user_data) return;
    SystemMenuData* data = (SystemMenuData*)menu->user_data;

    // Update system info first
    update_system_info(&data->info);

    // Update labels directly in the menu's item array
    // Ensure sufficient buffer size
    char buffer[64];

    // Item 0: CPU
    snprintf(buffer, sizeof(buffer), "CPU: %.1f%%", data->info.cpu_usage);
    // Free old label if it exists and update
    free((void*)menu->config.items[0].label);
    menu->config.items[0].label = strdup(buffer);

    // Item 1: Memory
    snprintf(buffer, sizeof(buffer), "Memory: %.1f%%", data->info.mem_usage);
    free((void*)menu->config.items[1].label);
    menu->config.items[1].label = strdup(buffer);

    // Item 2: Processes
    snprintf(buffer, sizeof(buffer), "Processes: %u", data->info.proc_count);
    free((void*)menu->config.items[2].label);
    menu->config.items[2].label = strdup(buffer);

    // Item 3: Top Process
    snprintf(buffer, sizeof(buffer), "Top: %s", data->info.top_process);
    free((void*)menu->config.items[3].label);
    menu->config.items[3].label = strdup(buffer);

    // Optional: Mark menu for redraw if needed by the rendering system
    // menu_needs_redraw(menu);
}

/* Menu action callback */
static void system_menu_action(void* user_data) {
    // No action needed, just display
}

// Cleanup function for SystemMenuData and associated menu items
static void system_menu_cleanup(void* user_data) {
    SystemMenuData* data = (SystemMenuData*)user_data;
    if (!data) return;

    // Note: Menu items themselves are freed by menu_destroy,
    // but we need to free the dynamically allocated id and label strings within them.
    // This cleanup assumes it's called *before* the menu structure itself is freed,
    // allowing access to menu->config.items. If called after, this part needs adjustment.
    // A safer approach might be to store pointers to allocated strings within SystemMenuData.
    // However, for this example, we assume menu_destroy calls this first.

    // For now, we assume menu_destroy handles freeing the item array itself.
    // We only free the SystemMenuData struct.
    free(data);
}


// Update callback wrapper
static void system_menu_update_cb(Menu* menu, void* user_data) {
     update_menu_items(menu); // Pass the menu itself
}


// Creates the system monitor menu.
Menu* create_system_monitor_menu(void) {
    SystemMenuData* data = calloc(1, sizeof(SystemMenuData));
    if (!data) return NULL;

    data->update_interval = 1000; // ms
    data->item_count = 4;

    // Initial system info fetch
    update_system_info(&data->info);

    // Create menu items dynamically
    MenuItem* items = calloc(data->item_count, sizeof(MenuItem));
    if (!items) {
        free(data);
        return NULL;
    }

    char id_buffer[32];
    char label_buffer[64];

    // Item 0: CPU
    snprintf(id_buffer, sizeof(id_buffer), "sys_cpu");
    snprintf(label_buffer, sizeof(label_buffer), "CPU: %.1f%%", data->info.cpu_usage);
    items[0] = (MenuItem){ .id = strdup(id_buffer), .label = strdup(label_buffer), .action = system_menu_action };

    // Item 1: Memory
    snprintf(id_buffer, sizeof(id_buffer), "sys_mem");
    snprintf(label_buffer, sizeof(label_buffer), "Memory: %.1f%%", data->info.mem_usage);
    items[1] = (MenuItem){ .id = strdup(id_buffer), .label = strdup(label_buffer), .action = system_menu_action };

    // Item 2: Processes
    snprintf(id_buffer, sizeof(id_buffer), "sys_proc");
    snprintf(label_buffer, sizeof(label_buffer), "Processes: %u", data->info.proc_count);
    items[2] = (MenuItem){ .id = strdup(id_buffer), .label = strdup(label_buffer), .action = system_menu_action };

    // Item 3: Top Process
    snprintf(id_buffer, sizeof(id_buffer), "sys_top");
    snprintf(label_buffer, sizeof(label_buffer), "Top: %s", data->info.top_process);
    items[3] = (MenuItem){ .id = strdup(id_buffer), .label = strdup(label_buffer), .action = system_menu_action };

    // Check allocations
    for(size_t i = 0; i < data->item_count; ++i) {
        if (!items[i].id || !items[i].label) {
             for(size_t j = 0; j < data->item_count; ++j) { free((void*)items[j].id); free((void*)items[j].label); }
             free(items);
             free(data);
             return NULL;
        }
    }


    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4, // Super key
        .trigger_key = 39,         // 's' key
        .title = "System Monitor",
        .items = items,
        .item_count = data->item_count,
        .nav = {
            .next = { .key = 44, .label = "j" },
            .prev = { .key = 45, .label = "k" }
        },
        .act = {
            .activate_on_mod_release = false,
            .activate_on_direct_key = false
        }
        // Add style if desired
    };

    Menu* menu = menu_create(&config);

    // Free the temporary items array and its contents (id/label)
    for (size_t i = 0; i < data->item_count; i++) {
        free((void*)items[i].id);
        free((void*)items[i].label);
    }
    free(items);

    if (!menu) {
        free(data);
        return NULL;
    }

    // Assign user data, update callback, and cleanup callback
    menu->user_data = data;
    menu->update_cb = system_menu_update_cb;
    menu->cleanup_cb = system_menu_cleanup;
    menu_set_update_interval(menu, data->update_interval);

    return menu;
}

/* Example usage */
// Example main function
int main(void) {
    InputHandler *handler = input_handler_create();
    if (!handler) {
        fprintf(stderr, "Failed to create input handler\n");
        return 1;
    }
    if (!input_handler_setup_x(handler)) {
         fprintf(stderr, "Failed to setup X for input handler\n");
         input_handler_destroy(handler);
         return 1;
    }

    Menu* sys_menu = create_system_monitor_menu();
    if (!sys_menu) {
        fprintf(stderr, "Failed to create system monitor menu\n");
        input_handler_destroy(handler);
        return 1;
    }

    input_handler_add_menu(handler, sys_menu);

    printf("System monitor menu created and registered.\n");
    printf("Press Super+S to activate.\n");
    printf("Press ESC or q to exit.\n");

    input_handler_run(handler);

    printf("Exiting...\n");
    input_handler_destroy(handler); // Destroys handler and registered menus

    return 0;
}
