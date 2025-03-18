
/* menu.c */
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>

MenuContext *menu_create(X11FocusContext *focus_ctx, MenuConfig config) {
  xcb_connection_t *conn = focus_ctx->conn;
  MenuContext *ctx = calloc(1, sizeof(MenuContext));
  if (!ctx)
    return NULL;

  ctx->conn = conn;
  ctx->config = config;
  ctx->active = false;

  return ctx;
}

void menu_destroy(MenuContext *ctx) {
  if (!ctx)
    return;
  if (ctx->config.cleanup_cb)
    ctx->config.cleanup_cb(ctx->config.user_data);
  free(ctx);
}

bool menu_handle_key_press(MenuContext *ctx, xcb_key_press_event_t *event) {
  if (!ctx || !ctx->active)
    return false;
  return ctx->config.action_cb
             ? ctx->config.action_cb(event->detail, ctx->config.user_data)
             : false;
}

bool menu_handle_key_release(MenuContext *ctx, xcb_key_release_event_t *event) {
  (void)ctx;
  (void)event;
  // Current implementation does not explicitly handle key release actions
  // beyond menu deactivation
  return false;
}
