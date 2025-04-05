/* input_handler.c - Complete and updated Input handling implementation */
#include "input_handler.h"
#include "cairo_menu.h" // Include cairo_menu.h for menu_setup_cairo
#include "menu_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h> // For usleep
#include <xcb/xcb_ewmh.h>

#ifdef MENU_DEBUG
#define LOG_PREFIX "[INPUT]"
#endif
#include "log.h"

// Helper function to map keycodes to modifier masks
static uint16_t keycode_to_modifier_mask(uint8_t keycode) {
  if (keycode == 133)
    return XCB_MOD_MASK_4; // Super/Mod4 typically
  if (keycode == 64)
    return XCB_MOD_MASK_1; // Alt/Mod1 typically
  if (keycode == 37)
    return XCB_MOD_MASK_CONTROL; // Ctrl
  if (keycode == 50)
    return XCB_MOD_MASK_SHIFT; // Shift
  return 0;                    // Not a known modifier keycode
}

static uint16_t modifier_mask_to_keycode(uint16_t modifier) {
  if (modifier == XCB_MOD_MASK_4)
    return 133; // Super/Mod4 typically
  if (modifier == XCB_MOD_MASK_1)
    return 64; // Alt/Mod1 typically
  if (modifier == XCB_MOD_MASK_CONTROL)
    return 37; // Ctrl
  if (modifier == XCB_MOD_MASK_SHIFT)
    return 50; // Shift
  return 0;    // Not a known modifier mask
}

// Checks if the released key corresponds to a modifier included in menu_mod
static bool is_modifier_release(uint8_t keycode, uint16_t menu_mod) {
  /* uint16_t menu_mod_keycode = modifier_mask_to_keycode(menu_mod); */
  uint16_t released_mod_mask = keycode_to_modifier_mask(keycode);
  // Check if the released key is a known modifier AND if that modifier is
  // part of the menu_mod mask
  LOG("[IH-RELEASE] released_mod_mask=0x%x, menu_mod=0x%x", released_mod_mask,
      menu_mod);
  /* return released_mod_mask != 0 && menu_mod == released_mod_mask; */
  return released_mod_mask != 0 &&
         (menu_mod & released_mod_mask) == released_mod_mask;
}

/* static bool match_key(Menu *menu, struct timeval *last_update, */
/*                       void *user_data) { */
/*   xcb_key_press_event_t *ev = (xcb_key_press_event_t *)user_data; */

/*   if (menu->config.mod_key == ev->state && */
/*       menu->config.trigger_key == ev->detail) { */
/*     menu_manager_activate(manager, menu) { */
/*       return false; // Stop iteration */
/*     } */
/*     return true; // Continue iteration */
/*   } */
/* } */

InputHandler *input_handler_create() {
  InputHandler *handler = calloc(1, sizeof(InputHandler));
  if (!handler) {
    // calloc failed, handler is NULL, nothing to free
    return NULL;
  }
  handler->menu_manager = menu_manager_create();
  if (!handler->menu_manager) {
    free(handler); // Free handler if menu manager creation fails
    return NULL;
  }
  return handler;
}

bool input_handler_setup_x(InputHandler *handler) {
  xcb_connection_t *conn = NULL;
  xcb_ewmh_connection_t *ewmh = NULL;
  xcb_screen_t *screen = NULL;
  xcb_window_t root = XCB_NONE;

  // Retry connection logic for robustness, especially under Xvfb
  int retries = 7;
  conn = xcb_connect(NULL, NULL);
  while ((!conn || xcb_connection_has_error(conn)) && retries-- > 0) {
    if (conn)
      xcb_disconnect(conn); // Disconnect previous failed attempt
    conn = NULL;            // Ensure conn is NULL if connect fails below
    fprintf(stderr,
            "[WARN] Failed to connect to X server, retrying (%d left)...\n",
            retries + 1);
    usleep(500000); // Wait 500ms
    conn = xcb_connect(NULL, NULL);
  }

  if (!conn || xcb_connection_has_error(conn)) {
    fprintf(stderr,
            "[ERROR] Failed to connect to X server after multiple retries.\n");
    if (conn)
      xcb_disconnect(conn); // Clean up the last failed attempt
    goto fail_conn;         // Use existing cleanup path
  }

  screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  if (!screen) {
    fprintf(stderr, "[ERROR] Failed to get X screen\n");
    goto fail_conn;
  }
  root = screen->root;

  ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
  if (!ewmh) {
    fprintf(stderr, "[ERROR] Failed to allocate EWMH structure\n");
    goto fail_conn;
  }

  if (!xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(conn, ewmh),
                                   NULL)) {
    fprintf(stderr, "[ERROR] Failed to initialize EWMH\n");
    goto fail_conn;
  }

  handler->conn = conn;
  handler->screen = screen;
  handler->ewmh = ewmh; // Assign early, cleanup logic will handle it

  // Set up focus context and assign to handler
  handler->focus_ctx = x11_focus_init(conn, root, ewmh);
  if (!handler->focus_ctx) {
    fprintf(stderr, "[ERROR] Failed to create focus context\n");
    goto fail_ewmh;
  }

  handler->root = malloc(sizeof(xcb_window_t));
  if (!handler->root) {
    fprintf(stderr, "[ERROR] Failed to allocate memory for root\n");
    goto fail_focus;
  }
  *handler->root = root;
  // Line 85: handler->root = &root; // REMOVED - was assigning stack address

  handler->modifier_mask = 0; // Initialize modifier mask

  menu_manager_connect(handler->menu_manager, conn, handler->focus_ctx, ewmh);
  // x11_set_window_floating(handler->focus_ctx, root); // Is this needed
  // here? Maybe in menu activation?

  if (!x11_grab_inputs(handler->focus_ctx, root)) {
    fprintf(stderr, "[INPUT] Failed to grab inputs\n");
    goto fail_focus; // Cleanup focus context and root window memory
  }

  // Additional initialization steps...
  LOG("[SETUP] Input handler X setup successful");
  return true; // Success

fail_focus:
  if (handler->focus_ctx) {
    // Assuming cleanup handles release implicitly
    x11_focus_cleanup(handler->focus_ctx);
    handler->focus_ctx = NULL; // Prevent double free in destroy
  }
  free(handler->root); // Free root window memory if allocated
  handler->root = NULL;
fail_ewmh:
  // Focus context cleanup already happened or wasn't needed
fail_conn:
  if (ewmh) {
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    handler->ewmh = NULL; // Prevent double free in destroy
  }
  if (conn) {
    xcb_disconnect(conn);
    handler->conn = NULL; // Prevent double free in destroy
  }
  // Note: handler itself is not freed here, caller should handle it
  // menu_manager is also not destroyed here, handled by input_handler_destroy
  LOG("[SETUP] Input handler X setup failed");
  return false; // Failure
}
/* void input_handler_setup_x(InputHandler *handler) { */
/*   xcb_connection_t *conn = xcb_connect(NULL, NULL); */
/*   if (xcb_connection_has_error(conn)) { */
/*     free(handler); */
/*     LOG("[ERROR] Failed to connect to X server"); */
/*     goto fail; */
/*   } */
/*   LOG("[XCB] Connected to X server"); */

/*   xcb_screen_t *screen =
 * xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
 */
/*   xcb_window_t root = screen->root; */
/*   xcb_ewmh_connection_t *ewmh = calloc(1, sizeof(xcb_ewmh_connection_t));
 */
/*   if (!xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(conn, ewmh),
 */
/*                                    NULL)) { */
/*     LOG("[ERROR] Failed to initialize EWMH"); */
/*     xcb_ewmh_connection_wipe(ewmh); */
/*     xcb_disconnect(conn); */
/*     free(ewmh); */
/*     goto fail; */
/*   } */
/*   LOG("[XCB] EWMH initialized"); */

/*   LOG("[INIT-X] Connecting input handler"); */
/*   if (!handler) { */
/*     goto fail; */
/*   } */
/*   LOG("[INIT] Input handler ready"); */
/*   if (!conn || root == XCB_NONE) { */
/*     LOG("[ERROR] Failed to create input handler"); */
/*     xcb_ewmh_connection_wipe(ewmh); */
/*     xcb_disconnect(conn); */
/*     free(ewmh); */
/*     goto fail; */
/*   } */
/*   handler->screen = screen; */
/*   handler->conn = conn; */
/*   handler->root = &root; // Set root root */
/*   handler->modifier_mask = 0; */
/*   handler->focus_ctx = x11_focus_init(conn, ewmh); */
/*   handler->ewmh = ewmh; */

/*   if (!handler->focus_ctx) */
/*     goto fail; */

/*   if (!handler->menu_manager) */
/*     goto fail; */
/*   menu_manager_connect(handler->menu_manager, conn, handler->focus_ctx,
 * ewmh); */
/*   x11_set_window_floating(handler->focus_ctx, root); */
/*   if (!x11_grab_inputs(handler->focus_ctx, root)) { */
/*     fprintf(stderr, "[INPUT] Failed to grab inputs\n"); */
/*     goto fail; */
/*   } */
/*   /\* if (cookie) *\/ */
/*   /\*   free(cookie); *\/ */

/*   return; */

/* fail: */
/*   if (handler) { */
/*     input_handler_destroy(handler); */
/*   } */
/*   if (handler->menu_manager) */
/*     menu_manager_destroy(handler->menu_manager); */
/*   if (handler->focus_ctx) */
/*     x11_focus_cleanup(handler->focus_ctx); */

/*   if (ewmh) { */
/*     xcb_ewmh_connection_wipe(ewmh); */
/*   } */
/*   if (ewmh) { */
/*     free(ewmh); */
/*   } */
/*   if (conn) { */
/*     xcb_disconnect(conn); */
/*     free(conn); */
/*   } */
/*   return; */
/* } */

void input_handler_destroy(InputHandler *handler) {
  if (!handler)
    return;
  LOG("[DESTROY] Destroying input handler:root");

  if (handler->root) {
    free(handler->root); // Free the allocated memory for root window ID
    handler->root = NULL;
  }

  LOG("[DESTROY] Destroying input handler:focus");
  if (handler->focus_ctx) {
    // Assuming x11_focus_cleanup also handles releasing inputs if grabbed
    x11_focus_cleanup(handler->focus_ctx);
    // No need to free handler->focus_ctx itself if cleanup handles it,
    // otherwise free(handler->focus_ctx);
    handler->focus_ctx = NULL;
  }

  LOG("[DESTROY] Destroying input handler:ewmh");
  if (handler->ewmh) {
    xcb_ewmh_connection_wipe(handler->ewmh);
    free(handler->ewmh);
    handler->ewmh = NULL;
  }

  LOG("[DESTROY] Destroying input handler:menumgr");
  if (handler->menu_manager) {
    LOG("[DESTROY] Destroying menu manager");
    menu_manager_destroy(handler->menu_manager); // Now destroying the manager
    handler->menu_manager = NULL;
  }

  LOG("[DESTROY] Destroying input handler:conn");
  if (handler->conn) {
    // No need to check for errors, just disconnect if it exists
    xcb_disconnect(handler->conn);
    handler->conn = NULL;
  }

  // LOG("[DESTROY] Destroying input handler:activation");
  // if (handler->activation_states) { // Removed - field seems
  // unused/unallocated
  //   free(handler->activation_states);
  // }

  LOG("[DESTROY] Destroying input handler:handler");
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
      if (handler->menu_manager->active_menu) {
        menu_cancel(handler->menu_manager->active_menu);
        menu_manager_deactivate(handler->menu_manager);
      }
      return true; // ESC or 'q'
    }
    // run_mode == 0: prio to menu switching
    // run_mode == 1: prio to active menu
    int run_mode = 0;
    if (run_mode == 0) {
      Menu *menu_to_activate =
          input_handler_handle_activation(handler, kp->state, kp->detail);
      if (menu_to_activate) {
        LOG("[IH-PRESS] Menu_To_Activate: %s, Already Active?: %d",
            menu_to_activate->config.title,
            (handler->menu_manager->active_menu == menu_to_activate));
        if (handler->menu_manager->active_menu != menu_to_activate) {
          if (handler->menu_manager->active_menu) {
            menu_manager_deactivate(handler->menu_manager);
          }
          if (!menu_cairo_is_setup(menu_to_activate)) {
            menu_setup_cairo(handler->conn, *handler->root, handler->focus_ctx,
                             handler->screen, menu_to_activate);
          }
          menu_manager_activate(handler->menu_manager, menu_to_activate);
          return false;
        } else {
          return menu_handle_key_press(handler->menu_manager->active_menu, kp);
        }
      } else {
        if (handler->menu_manager->active_menu) {
          return menu_handle_key_press(handler->menu_manager->active_menu, kp);
          /* handler->menu_manager->active_menu */
        }
      }
    } else {
      // run_mode == 1: prio to active menu
      if (handler->menu_manager->active_menu) {
        if (menu_handle_key_press(handler->menu_manager->active_menu, kp)) {
          return true;
        }
      }
      Menu *menu_to_activate =
          input_handler_handle_activation(handler, kp->state, kp->detail);
      if (menu_to_activate) {
        if (handler->menu_manager->active_menu != menu_to_activate) {
          if (handler->menu_manager->active_menu) {
            menu_manager_deactivate(handler->menu_manager);
          }
          if (!menu_cairo_is_setup(menu_to_activate)) {
            menu_setup_cairo(handler->conn, *handler->root, handler->focus_ctx,
                             handler->screen, menu_to_activate);
          }
          menu_manager_activate(handler->menu_manager, menu_to_activate);
        }
      }
      return false;
    }
  }

  case XCB_FOCUS_IN: {
    xcb_focus_in_event_t *focus_event = (xcb_focus_in_event_t *)event;
    LOG("[IH-FOCUS] Focus in event: %d", focus_event->event);
    return false;
  }
  case XCB_KEY_RELEASE: {
    xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
    LOG("[IH-RELEASE]  release: code=%u, state=0x%x, globstate=0x%x",
        kr->detail, kr->state, handler->modifier_mask);

    /* menu_manager_handle_key_release(handler->menu_manager, kr); TODO
     * maybe later*/
    Menu *active_menu = handler->menu_manager->active_menu;
    if (active_menu) {
      uint16_t menu_mod = active_menu->config.mod_key;
      if (is_modifier_release(kr->detail, menu_mod)) {
        menu_confirm_selection(handler->menu_manager->active_menu);
        LOG("[IH-RELEASE] Exiting because menu modifier (0x%x) "
            "released (key %u)",
            menu_mod, kr->detail);
        menu_manager_deactivate(handler->menu_manager);
        return true;
      }
    }
    LOG("[IH-RELEASE] FINALIZING,exit=%d", false);
    return false;
  }
  default:
    LOG("Unhandled event type: 0x%x", type);
    return false;
  }
}

// Adds an already created menu to the input handler's menu manager.
Menu *input_handler_add_menu(InputHandler *handler, Menu *menu) {
  if (!handler || !menu || !handler->menu_manager) {
    // Return NULL if handler, menu, or manager is invalid
    return NULL;
  }
  LOG("[HANDLER->MANAGER] Adding menu: [%s]", menu->config.title);

  // Register the provided menu with the manager
  if (menu_manager_register(handler->menu_manager, menu)) {
    return menu; // Return the menu if registration is successful
  } else {
    LOG("[ERROR] Failed to register menu: [%s]", menu->config.title);
    return NULL; // Return NULL if registration fails
  }
  // Note: Ownership of the 'menu' pointer is effectively transferred
  // to the menu_manager upon successful registration.
}

// TODO any use?
/* bool input_handler_remove_menu(InputHandler *handler, Menu *menu) { */
/*   if (!handler || !menu) */
/*     return false; */
/*   LOG("[HANDLER->MANAGER] Rem menu: [%s]", menu->config.title); */
/*   menu_manager_unregister(handler->menu_manager, menu); */
/*   return true; */
/* } */
Menu *input_handler_handle_activation(InputHandler *handler, uint16_t mod_key,
                                      uint8_t keycode) {
  for (size_t i = 0; i < menu_manager_get_menu_count(handler->menu_manager);
       i++) {
    Menu *menu = menu_manager_menu_index(handler->menu_manager, i);
    LOG("Checking activation state: mod_key=0x%x, keycode=%u", mod_key,
        keycode);
    if (menu->config.mod_key == mod_key &&
        menu->config.trigger_key == keycode) {
      LOG("[%s] Activation state matched: mod_key=0x%x, keycode=%u",
          menu->config.title, menu->config.mod_key, menu->config.trigger_key);
      return menu;
    }
  }
  return NULL;
}
