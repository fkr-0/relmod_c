/* system_monitor_menu.c - Example system monitor menu implementation */
#include "../src/menu.h"
#include "../src/cairo_menu.h"
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
    MenuItem* items;
    size_t item_count;
    unsigned int update_interval;  /* milliseconds */
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
            // Check if entry is a directory and name is numeric (PID)
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
            name[strcspn(name, "\n")] = 0;  // Remove newline
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
static void update_menu_items(SystemMenuData* data) {
    char* labels[] = {
        malloc(64), malloc(64), malloc(64), malloc(64)
    };
    
    snprintf(labels[0], 64, "CPU: %.1f%%", data->info.cpu_usage);
    snprintf(labels[1], 64, "Memory: %.1f%%", data->info.mem_usage);
    snprintf(labels[2], 64, "Processes: %u", data->info.proc_count);
    snprintf(labels[3], 64, "Top: %s", data->info.top_process);
    
    for (size_t i = 0; i < 4; i++) {
        data->items[i].label = labels[i];
        if (i > 0) free((char*)data->items[i].label);  // Free old label
    }
}

/* Menu action callback */
static void system_menu_action(void* user_data) {
    // No action needed, just display
}

/* Create system monitor menu */
Menu* create_system_monitor_menu(xcb_connection_t* conn, xcb_window_t root) {
    SystemMenuData* data = calloc(1, sizeof(SystemMenuData));
    if (!data) return NULL;
    
    data->update_interval = 1000;  // 1 second update
    data->item_count = 4;
    data->items = calloc(data->item_count, sizeof(MenuItem));
    
    if (!data->items) {
        free(data);
        return NULL;
    }
    
    // Initialize items
    for (size_t i = 0; i < data->item_count; i++) {
        data->items[i].id = malloc(32);
        snprintf((char*)data->items[i].id, 32, "sys_%zu", i);
        data->items[i].action = system_menu_action;
        data->items[i].metadata = data;
    }
    
    // Initial update
    update_system_info(&data->info);
    update_menu_items(data);
    
    MenuConfig config = {
        .mod_key = XCB_MOD_MASK_4,    /* Super key */
        .trigger_key = 39,             /* 's' key */
        .title = "System Monitor",
        .items = data->items,
        .item_count = data->item_count,
        .nav = {
            .next = { .key = 44, .label = "j" },  /* j key */
            .prev = { .key = 45, .label = "k" }   /* k key */
        },
        .act = {
            .activate_on_mod_release = false,
            .activate_on_direct_key = false
        }
    };
    
    return cairo_menu_create(conn, root, &config);
}

/* Example usage */
int main(void) {
    xcb_connection_t* conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to X server\n");
        return 1;
    }
    
    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    Menu* menu = create_system_monitor_menu(conn, screen->root);
    
    if (!menu) {
        fprintf(stderr, "Failed to create system monitor menu\n");
        xcb_disconnect(conn);
        return 1;
    }
    
    printf("System monitor menu created.\n");
    printf("Press Super+S to show menu.\n");
    
    // Main event loop would go here
    
    // Cleanup
    menu_destroy(menu);
    xcb_disconnect(conn);
    
    return 0;
}