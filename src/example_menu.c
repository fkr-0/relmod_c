/* example_menu.c */
#include "example_menu.h"
#include <stdio.h>
#include <stdlib.h>

// User data struct to hold menu state (if needed)
typedef struct {
  int events_logged;
  uint16_t modifier_mask;  // Store the modifier mask
} ExampleMenuData;

static bool example_menu_activates(uint16_t modifiers, uint8_t keycode,
                                   void *user_data) {
  ExampleMenuData *data = user_data;
  printf("[ACTIVATE CHECK] modifiers=0x%x, keycode=%d\n", modifiers, keycode);
  return (modifiers & data->modifier_mask) == data->modifier_mask; // Check against stored mask
}

static bool example_menu_action(uint8_t keycode, void *user_data) {
  ExampleMenuData *data = user_data;
  data->events_logged++;
  printf("[ACTION] keycode=%d, events_logged=%d\n", keycode,
         data->events_logged);
  return true; // stay active
}

void example_menu_cleanup(void *user_data) {
  ExampleMenuData *data = user_data;
  printf("[CLEANUP] total_events_logged=%d\n", data->events_logged);
  free(data);
}

MenuConfig example_menu_create(uint16_t modifier_mask) {
  ExampleMenuData *data = calloc(1, sizeof(ExampleMenuData));
  data->modifier_mask = modifier_mask;  // Store the modifier mask

  return (MenuConfig){
    .mod_key = modifier_mask,
    .trigger_key = 31,  // Example trigger key
    .title = "Example Menu",
    .items = NULL,  // Add menu items here
    .item_count = 0,  // Update with the number of items
    .nav = {
      .next = { .key = 44, .label = "j" },
      .prev = { .key = 45, .label = "k" }
    },
    .act = {
      .activate_on_mod_release = true,
      .activate_on_direct_key = true
    },
    .style = {
      .background_color = {0.1, 0.1, 0.1, 0.9},
      .text_color = {0.8, 0.8, 0.8, 1.0},
      .highlight_color = {0.3, 0.3, 0.8, 1.0},
      .font_face = "Sans",
      .font_size = 14.0,
      .item_height = 20,
      .padding = 10
    }
  };
}
