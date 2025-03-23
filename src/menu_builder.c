// =================================== FILE: menu_builder.c
// ===================================
#include "menu_builder.h"
#include <stdlib.h>
#include <string.h>

MenuBuilder menu_builder_create(const char *title, size_t capacity) {
  MenuBuilder builder = {0};
  builder.capacity = capacity;
  builder.count = 0;
  builder.items = calloc(capacity, sizeof(MenuItem));
  if (!builder.items) {
    builder.capacity = 0;
    return builder;
  }

  builder.config.title = strdup(title);
  builder.config.items = builder.items;
  builder.config.item_count = 0;

  builder.config.act.activate_on_mod_release = true;
  builder.config.act.activate_on_direct_key = true;
  return builder;
}

bool menu_builder_add_item(MenuBuilder *builder, const char *label,
                           void (*action)(void *), void *metadata) {
  if (!builder || !label || builder->count >= builder->capacity)
    return false;

  MenuItem *item = &builder->items[builder->count++];
  item->label = strdup(label);
  item->action = action;
  item->metadata = metadata;

  builder->config.item_count = builder->count;
  return true;
}

// Setter for ActivationState in MenuConfig
void menu_builder_set_activation_state(MenuBuilder *builder, uint16_t mod_key,
                                       uint8_t keycode) {
  builder->config.act_state.config =
      &builder->config; // point back to the MenuConfig itself
  builder->config.act_state.mod_key = mod_key;
  builder->config.act_state.keycode = keycode;
  builder->config.act_state.initialized = false;
  builder->config.act_state.menu = NULL; // menu initialized later
}

MenuConfig *menu_builder_finalize(MenuBuilder *builder) {
  if (!builder || builder->count == 0)
    return NULL;

  MenuConfig *config = calloc(1, sizeof(MenuConfig));
  if (!config)
    return NULL;

  config->title = strdup(builder->config.title);
  config->items = calloc(builder->count, sizeof(MenuItem));
  if (!config->items) {
    free((char *)config->title);
    free(config);
    return NULL;
  }

  for (size_t i = 0; i < builder->count; i++) {
    config->items[i].label = strdup(builder->items[i].label);
    config->items[i].action = builder->items[i].action;
    config->items[i].metadata = builder->items[i].metadata;
  }
  config->item_count = builder->count;
  config->act = builder->config.act;
  config->nav = builder->config.nav;
  config->style = builder->config.style;
  config->mod_key = builder->config.mod_key;
  config->trigger_key = builder->config.trigger_key;
  // Correct setup in finalize:
  config->act_state.config = config; // Correct: self-reference for config
  config->act_state.menu = NULL;     //  initialized later at activation time
  config->act_state.initialized = false;
  config->act_state.mod_key = builder->config.mod_key;
  config->act_state.keycode = builder->config.trigger_key;

  return config;
}

// Cleanly destroy a finalized MenuConfig
void menu_config_destroy(MenuConfig *config) {
  if (!config)
    return;

  // Free duplicated title
  if (config->title)
    free((char *)config->title);

  // Free duplicated item labels and items array
  if (config->items) {
    for (size_t i = 0; i < config->item_count; i++)
      free((char *)config->items[i].label);
    free(config->items);
  }

  // Free dynamically allocated navigation direct keys if applicable
  if (config->nav.direct.keys)
    free(config->nav.direct.keys);

  // Finally, free the config itself
  free(config);
}

void menu_builder_destroy(MenuBuilder *builder) {
  if (!builder || !builder->items)
    return;

  for (size_t i = 0; i < builder->count; i++)
    free((char *)builder->items[i].label);

  free(builder->items);
  builder->items = NULL;
  free((char *)builder->config.title);

  builder->count = 0;
  builder->capacity = 0;
}

// Implementation of missing setter functions
void menu_builder_set_mod_key(MenuBuilder *builder, uint16_t mod_key) {
  builder->config.mod_key = mod_key;
}

void menu_builder_set_trigger_key(MenuBuilder *builder, uint8_t trigger_key) {
  builder->config.trigger_key = trigger_key;
}

void menu_builder_set_navigation_keys(MenuBuilder *builder, uint8_t next_key,
                                      const char *next_label, uint8_t prev_key,
                                      const char *prev_label,
                                      uint8_t *direct_keys,
                                      size_t direct_count) {
  builder->config.nav.next.key = next_key;
  builder->config.nav.next.label = next_label;
  builder->config.nav.prev.key = prev_key;
  builder->config.nav.prev.label = prev_label;
  builder->config.nav.direct.keys = calloc(direct_count, sizeof(uint8_t));
  memcpy(builder->config.nav.direct.keys, direct_keys, direct_count);
  builder->config.nav.direct.count = direct_count;
}

void menu_builder_set_activation(MenuBuilder *builder, bool on_mod_release,
                                 bool on_direct_key) {
  builder->config.act.activate_on_mod_release = on_mod_release;
  builder->config.act.activate_on_direct_key = on_direct_key;
}
// ==============================================================================================
