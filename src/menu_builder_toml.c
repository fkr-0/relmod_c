#ifdef ENABLE_MENU_TOML

#include "menu_builder.h"
#include <stdio.h>
#include <string.h>
#include <toml.h>

Menu *menu_builder_load_from_toml(const char *filename,
                                  void (*default_action)(void *)) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "TOML: Could not open file: %s\n", filename);
    return NULL;
  }

  char errbuf[200];
  toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
  fclose(fp);

  if (!conf) {
    fprintf(stderr, "TOML: Parse error: %s\n", errbuf);
    return NULL;
  }

  const char *title = toml_raw_in(conf, "title");
  char *title_str = NULL;
  if (title)
    toml_rtos(title, &title_str);
  if (!title_str)
    title_str = strdup("Untitled");

  toml_array_t *items = toml_array_in(conf, "items");
  if (!items || toml_array_kind(items) != 't') {
    fprintf(stderr, "TOML: Missing [[items]] table\n");
    toml_free(conf);
    free(title_str);
    return NULL;
  }

  size_t count = toml_array_nelem(items);
  MenuBuilder builder = menu_builder_create(title_str, count);
  free(title_str);

  for (size_t i = 0; i < count; ++i) {
    toml_table_t *entry = toml_table_at(items, i);
    const char *label_raw = toml_raw_in(entry, "label");

    char *label = NULL;
    if (label_raw)
      toml_rtos(label_raw, &label);
    if (!label)
      label = strdup("Unnamed");

    menu_builder_add_item(&builder, label, default_action, NULL);
    free(label);
  }

  toml_free(conf);
  Menu *menu = menu_builder_finalize(&builder);
  menu_builder_destroy(&builder);
  return menu;
}

#endif
