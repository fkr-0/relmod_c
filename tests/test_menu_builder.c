#include "menu_builder.h"
#include <assert.h>
#include <stdio.h>

static int called = 0;
void dummy_action(void *data) { called++; }

void test_basic_builder() {
  MenuBuilder b = menu_builder_create("Test", 3);
  assert(menu_builder_add_item(&b, "A", dummy_action, NULL));
  assert(menu_builder_add_item(&b, "B", dummy_action, NULL));
  assert(menu_builder_add_item(&b, "C", dummy_action, NULL));
  assert(!menu_builder_add_item(&b, "D", dummy_action, NULL)); // over capacity

  MenuConfig *menu = menu_builder_finalize(&b);
  assert(menu != NULL);
  assert(menu->item_count == 3);
  menu_builder_destroy(&b);
}

#ifdef ENABLE_MENU_TOML
void test_toml_loader() {
  called = 0;
  Menu *m = menu_builder_load_from_toml("menu_config.toml", dummy_action);
  assert(m != NULL);
  assert(m->config.item_count == 3);
  MenuItem *item = &m->config.items[2];
  assert(strcmp(item->label, "Quit") == 0);
  item->action(item->metadata);
  assert(called == 1);
  menu_destroy(m);
}
#endif

int main() {
  test_basic_builder();
#ifdef ENABLE_MENU_TOML
  test_toml_loader();
#endif
  printf("All tests passed.\n");
  return 0;
}
