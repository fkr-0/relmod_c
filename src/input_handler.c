/* input_handler.c - Complete and updated Input handling implementation */
#include "input_handler.h"
#include "cairo_menu.h"        // Include cairo_menu.h for cairo_menu_create
#include "cairo_menu_render.h" // Include cairo_menu.h for cairo_menu_create
#include "menu_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[INPUT]"
#include "log.h"
#endif

static bool is_modifier_release(uint16_t state, uint8_t keycode) {
  return (state == 0x40 && keycode == 133) || // Super
         (state == 0x08 && keycode == 64) ||  // Alt
         (state == 0x04 && keycode == 37) ||  // Ctrl
         (state == 0x01 && keycode == 50);    // Shift
}

InputHandler *input_handler_create() {
  InputHandler *handler = calloc(1, sizeof(InputHandler));
  if (!handler) {
    free(handler);
    return NULL;
  }
  handler->menu_manager = menu_manager_create();
  return handler;
}

void input_handler_setup_x(InputHandler *handler) {

  xcb_connection_t *conn = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(conn)) {
    LOG("[ERROR] Failed to connect to X server");
    return;
  }
  LOG("[XCB] Connected to X server");

  xcb_ewmh_connection_t ewmh = {0};
  xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, cookie, NULL)) {
    LOG("[ERROR] Failed to initialize EWMH");
    xcb_disconnect(conn);
    return;
  }
  LOG("[XCB] EWMH initialized");

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  xcb_window_t root = screen->root;

  LOG("[INIT-X] Connecting input handler");
  if (!handler) {
    return;
  }
  LOG("[INIT] Input handler ready");
  if (!conn || root == XCB_NONE) {
    LOG("[ERROR] Failed to create input handler");
    xcb_ewmh_connection_wipe(&ewmh);
    xcb_disconnect(conn);
  }
  handler->screen = screen;
  handler->conn = conn;
  handler->root = &root; // Set root root
  handler->modifier_mask = 0;
  handler->focus_ctx = x11_focus_init(conn, &ewmh);

  if (!handler->focus_ctx)
    goto fail;

  if (!handler->menu_manager)
    goto fail;
  menu_manager_connect(handler->menu_manager, conn, handler->focus_ctx, &ewmh);
  x11_set_window_floating(handler->focus_ctx, root);
  if (!x11_grab_inputs(handler->focus_ctx, root)) {
    fprintf(stderr, "[INPUT] Failed to grab inputs\n");
    goto fail;
  }

  return;

fail:
  if (handler->menu_manager)
    menu_manager_destroy(handler->menu_manager);
  if (handler->focus_ctx)
    x11_focus_cleanup(handler->focus_ctx);
  free(handler);
  return;
}

void input_handler_destroy(InputHandler *handler) {
  if (!handler)
    return;
  if (handler->focus_ctx) {
    x11_release_inputs(handler->focus_ctx);
    x11_focus_cleanup(handler->focus_ctx);
  }
  if (handler->menu_manager)
    menu_manager_destroy(handler->menu_manager);
  free(handler->activation_states);
  free(handler);
}

static bool update_callback(Menu *menu, struct timeval *last_update,
                            void *user_data) {
  struct timeval now;
  gettimeofday(&now, NULL);

  if (!menu->active || menu->update_interval <= 0 || !menu->update_cb)
    return true;

  unsigned long elapsed = (now.tv_sec - last_update->tv_sec) * 1000 +
                          (now.tv_usec - last_update->tv_usec) / 1000;
  if (elapsed >= menu->update_interval) {
    menu_trigger_update(menu);
    *last_update = now;
  }
  return true;
}

void input_handler_run(InputHandler *handler) {
  if (!handler)
    return;

  int fd = xcb_get_file_descriptor(handler->conn);
  while (1) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval timeout = {0};
    timeout.tv_sec = 1; // fallback timeout

    int ret = select(fd + 1, &fds, NULL, NULL, &timeout);

    if (ret > 0) {
      xcb_generic_event_t *event;
      while ((event = xcb_poll_for_event(handler->conn))) {
        LOG("Incoming event");

        if (input_handler_handle_event(handler, event)) {
          LOG("Exiting loop");
          free(event);
          return;
        }
        free(event);
      }
    }

    menu_manager_foreach(handler->menu_manager, update_callback, NULL);

    if (xcb_connection_has_error(handler->conn)) {
      LOG("X11 connection error detected, exiting loop");
      break;
    }
  }
}

bool input_handler_process_event(InputHandler *handler) {
  if (!handler)
    return false;
  xcb_generic_event_t *event = xcb_poll_for_event(handler->conn);
  if (!event)
    return false;

  bool result = input_handler_handle_event(handler, event);
  free(event);
  return result;
}

bool input_handler_handle_event(InputHandler *handler,
                                xcb_generic_event_t *event) {
  if (!handler || !event)
    return false;

  uint8_t type = event->response_type & ~0x80;
  switch (type) {
  case XCB_KEY_PRESS: {
    xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
    LOG("Key press: code=%u, state=0x%x", kp->detail, kp->state);

    if (kp->detail == 9 || kp->detail == 24) {
      LOG("[IH-PRESS] Exit because esc/q | Modifier mask: 0x%x", kp->state);
      return true; // ESC or 'q'
    }

    if (handler->modifier_mask == 0 && kp->state != 0) {
      LOG("[IH-PRESS] Setting modmask because globmod=0 Modifier mask: 0x%x",
          kp->state);
      handler->modifier_mask = kp->state;
    }

    if (input_handler_handle_activation(handler, kp->state, kp->detail)) {
      LOG("[IH-PRESS] handle activation returned true, activating.");
    }

    LOG("[IH-PRESS] PASS event TO MENU MANAGER");
    return menu_manager_handle_key_press(handler->menu_manager, kp);
  }
  case XCB_KEY_RELEASE: {
    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
    LOG("[IH-RELEASE]  release: code=%u, state=0x%x, globstate=0x%x",
        kr->detail, kr->state, handler->modifier_mask);

    if (handler->modifier_mask == 0 && kr->state != 0) {
      handler->modifier_mask = kr->state;
      LOG("[IH-RELEASE]  set glob state because was 0: code=%u, state=0x%x, "
          "globstate=0x%x",
          kr->detail, kr->state, handler->modifier_mask);
    }

    LOG("[IH-RELEASE] PASS event TO MENU MANAGER");
    menu_manager_handle_key_release(handler->menu_manager, kr);
    bool ex = is_modifier_release(kr->state, kr->detail);
    LOG("[IH-RELEASE] FINALIZING,exit=%d", ex);
    return ex;
  }
  default:
    LOG("Unhandled event type: 0x%x", type);
    return false;
  }
}

bool input_handler_add_menu(InputHandler *handler, MenuConfig *config) {
  if (!handler || !config)
    return false;
  LOG("[HANDLER->MANAGER] Adding menu: [%s]", config->title);

  ActivationState state = config->act_state;
  /* state.config = config; */
  /* state.menu = menu; */
  input_handler_add_activation_state(handler, &state);
  return true;
}

bool input_handler_remove_menu(InputHandler *handler, Menu *menu) {
  if (!handler || !menu)
    return false;
  LOG("[HANDLER->MANAGER] Rem menu: [%s]", menu->config.title);
  menu_manager_unregister(handler->menu_manager, menu);
  return true;
}

bool input_handler_add_activation_state(InputHandler *handler,
                                        ActivationState *state) {
  if (!handler || !state)
    return false;
  handler->activation_states =
      realloc(handler->activation_states,
              sizeof(ActivationState) * (handler->activation_state_count + 1));
  if (!handler->activation_states)
    return false;
  handler->activation_state_count++;
  handler->activation_states[handler->activation_state_count - 1] = *state;
  return true;
}

/* Registry iteration
 * Calls the given function for each registered menu
 * Stops iteration if the callback returns false.
 * Usage:
 * menu_manager_foreach(manager, callback_fn, user_data);
 * Example callback_fn:
 * bool callback_fn(Menu *menu, struct timeval *last_update, void *user_data) {
 *  // Do something with menu, e.g. match ev->state + ev->detail
 * if (menu->mod_key == ev->state && menu->trigger_key == ev->detail) {
 *   menu_manager_activate(manager, menu);
 *   return false; // Stop iteration
 * }
 * return true; // Continue iteration
 * */
/* bool input_handler_handle_activation(InputHandler *handler, uint16_t mod_key,
 */
/*                                      uint8_t keycode) { */
/*   LOG("Handling activation: mod_key=0x%x, keycode=%u", mod_key, keycode); */

/*   // use */
/*   // void menu_manager_foreach(MenuManager *mgr, MenuManagerForEachFn fn, */
/*   // void *user_data) { */
/*   /\* const bool callback_fn(Menu * menu, struct timeval * last_update, *\/
 */
/*   /\*                        void *user_data) { *\/ */
/*   /\*   if (menu->config.act_state.mod_key == mod_key && *\/ */
/*   /\*       menu->config.act_state.keycode == keycode) { *\/ */
/*   /\*     LOG("Activation state matched: mod_key=0x%x, keycode=%u", mod_key,
 * *\/ */
/*   /\*         keycode); *\/ */
/*   /\*     menu_manager_activate(handler->menu_manager, menu); *\/ */
/*   /\*     return false; *\/ */
/*   /\*   } *\/ */
/*   /\*   return true; *\/ */
/*   /\* } *\/ */
/*   /\* menu_manager_foreach(handler->menu_manager, callback_fn, NULL); *\/ */

/*   for (size_t i = 0; i < handler->activation_state_count; i++) { */
/*     ActivationState *state = &handler->activation_states[i]; */
/*     LOG("Checking activation state: mod_key=0x%x, keycode=%u",
 * state->mod_key, */
/*         state->keycode); */
/*     if (state->mod_key == mod_key && state->keycode == keycode) { */
/*       LOG("Activation state matched: mod_key=0x%x, keycode=%u", mod_key, */
/*           keycode); */
/*       // LOG state */
/*       LOG("State: config=%p, mod_key=0x%x, keycode=%u, initialized=%d, " */
/*           "menu=%p, title=%s", */
/*           state->config, state->mod_key, state->keycode, state->initialized,
 */
/*           state->menu, state->config->title); */
/*       if (!state->initialized) { */
/*         MenuConfig *config = state->config; */
/*         /\* MenuConfig cfg = *config; *\/ */
/*         LOG("Initializing menu [%s] for activation state (window: %p)", */
/*             (config)->title, handler->root); */
/*         state->menu = cairo_menu_create(handler->conn, *handler->root, */
/*                                         state->config); // Use handler->root
 */
/*         menu_manager_register(handler->menu_manager, state->menu); */
/*         LOG("Menu created: %p Count :%ld\n", state->menu, */
/*             menu_manager_get_menu_count(handler->menu_manager)); */

/*         state->initialized = true; */
/*         menu_manager_activate(handler->menu_manager, state->menu); */
/*         LOG("Menu activated: %p", state->menu); */

/*         /\* cairo_menu_render_show(state->menu->user_data); *\/ */
/*         /\* state->menu->update_cb(state->menu, state->menu->user_data); *\/
 */
/*       } */
/*       return true; */
/*     } */
/*   } */
/*   LOG("No activation state matched: mod_key=0x%x, keycode=%u", mod_key, */
/*       keycode); */
/*   return false; */
/* } */

// Corrected activation logic (in your existing input handler)
bool input_handler_handle_activation(InputHandler *handler, uint16_t mod_key,
                                     uint8_t keycode) {
  for (size_t i = 0; i < handler->activation_state_count; i++) {
    ActivationState *state = &handler->activation_states[i];
    LOG("Checking activation state: mod_key=0x%x, keycode=%u", state->mod_key,
        keycode);
    if (state->mod_key == mod_key && state->keycode == keycode) {
      LOG("Activation state matched: mod_key=0x%x, keycode=%u", mod_key,
          keycode);

      LOG("Menu activating: %p", state->menu);
      LOG("Handler->root: %p", handler->root);
      LOG("Handler->conn: %p", handler->conn);
      /* state->menu = cairo_menu_create(handler->conn, handler->screen->root,
       */
      /*                                 state->config); */

      if (!state->menu || !state->menu->user_data) {
        state->menu = cairo_menu_create(handler->conn, *handler->root,
                                        handler->screen, state->config);
      }
      LOG("Menu activating: %p", state->menu);
      if (!state->menu || !state->menu->user_data) {
        fprintf(stderr, "Menu creation failed or user_data invalid\n");
        return false;
      }

      LOG("Menu created: %p Count :%ld\n Connection %p RenderWindow %u",
          state->menu, menu_manager_get_menu_count(handler->menu_manager),
          handler->conn,
          ((CairoMenuData *)state->menu->user_data)->render.window);
      //&((CairoMenuRenderData *)state->menu->user_data)->window);
      // Proper initialization here:

      state->initialized = true;
      menu_manager_register(handler->menu_manager, state->menu);
      menu_manager_activate(handler->menu_manager, state->menu);
      /* menu_show(state->menu); */
      /* cairo_menu_show(state->menu->user_data); */
      /* // Correct usage of show: */
      /* cairo_menu_render_show((CairoMenuData *)state->menu->user_data); */

      /* if (!state->initialized) { */
      /*   state->menu = cairo_menu_create(handler->conn,
       * handler->screen->root,
       */
      /*                                   state->config); */
      /*   state->initialized = (state->menu != NULL); */
      /*   if (state->initialized) { */
      /*     LOG("Menu created: %p", state->menu); */
      /*     menu_manager_register(handler->menu_manager, state->menu); */
      /*     menu_manager_activate(handler->menu_manager, state->menu); */
      /*     cairo_menu_show(state->menu->user_data); */
      /*     cairo_menu_render_show(state->menu->user_data); */
      /*   } */
      /* } */
      return true;
    }
  }
  return false;
}
