#include "menu_builder.h"
#include <stdlib.h>
#include <string.h>

MenuBuilder menu_builder_create(const char *title, size_t capacity) {
  MenuBuilder b = {0};
  b.capacity = capacity;
  b.count = 0;
  b.items = calloc(capacity, sizeof(MenuItem));
  if (!b.items) {
    b.capacity = 0;
    return b;
  }
  b.config.title = title;
  b.config.items = b.items;
  b.config.item_count = 0;
  b.config.act.activate_on_mod_release = true;
  b.config.act.activate_on_direct_key = true;
  return b;
}

bool menu_builder_add_item(MenuBuilder *b, const char *label,
                           void (*action)(void *), void *metadata) {
  if (!b || !label || !b->items || b->count >= b->capacity)
    return false;

  MenuItem *item = &b->items[b->count++];
  item->label = strdup(label); // own copy
  item->action = action;
  item->metadata = metadata;
  item->id = NULL; // optional

  b->config.item_count = b->count;
  return true;
}

Menu *menu_builder_finalize(MenuBuilder *b) {
  if (!b || b->count == 0)
    return NULL;

  Menu *menu = menu_create(&b->config);
  return menu;
}

void menu_builder_destroy(MenuBuilder *b) {
  if (!b || !b->items)
    return;

  for (size_t i = 0; i < b->count; i++) {
    free((char *)b->items[i].label); // strdup'ed earlier
  }
  free(b->items);
  b->items = NULL;
  b->count = 0;
  b->capacity = 0;
}
