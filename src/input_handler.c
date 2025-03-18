/* input_handler.c */
#include "input_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include "x11_focus.h"

// Helper: check if menu should activate based on event
static bool menu_should_activate(MenuContext *ctx, uint16_t modifiers,
                                 uint8_t keycode) {
    if (ctx->config.activates_cb) {
        return ctx->config.activates_cb(modifiers, keycode, ctx->config.user_data);
    }
    // Default: activate if modifier matches exactly
    return (modifiers & ctx->config.modifier_mask) == ctx->config.modifier_mask;
}

// Private helper function to handle events
static bool handle_event(InputHandler *handler, xcb_generic_event_t *event) {
    bool exit = false;
    printf("[DEBUG] Received event type: 0x%x\n", event->response_type & ~0x80);

    switch (event->response_type & ~0x80) {
    case XCB_KEY_PRESS: {
        xcb_key_press_event_t *kp = (xcb_key_press_event_t *)event;
        bool handled = false;

        printf("[DEBUG] Key press event - detail: %d, state: 0x%x\n", 
               kp->detail, kp->state);

        if (kp->detail == 9 && kp->state == 0) {
            // Escape key pressed without modifiers
            printf("[DEBUG] Escape key detected, exiting\n");
            exit = true;
            break;
        }
        // exit on q
        if (kp->detail == 24) {
            printf("[DEBUG] Q key detected, exiting\n");
            exit = true;
            break;
        }

        // First, check active menus
        for (size_t i = 0; i < handler->menu_count; i++) {
            if (handler->menus[i]->active) {
                if (menu_handle_key_press(handler->menus[i], kp)) {
                    handled = true;
                    break;
                }
            }
        }

        if (handled)
            break;

        // Next, check activation conditions
        for (size_t i = 0; i < handler->menu_count; i++) {
            if (!handler->menus[i]->active &&
                menu_should_activate(handler->menus[i], kp->state, kp->detail)) {
                handler->menus[i]->active = true;
                handler->menus[i]->active_modifier =
                    kp->state & handler->menus[i]->config.modifier_mask;
                printf("Menu %zu activated\n", i);
                break;
            }
        }
        break;
    }

    case XCB_KEY_RELEASE: {
        xcb_key_release_event_t *kr = (xcb_key_release_event_t *)event;
        printf("[DEBUG] Key release event - detail: %d, state: 0x%x\n", 
               kr->detail, kr->state);

        for (size_t i = 0; i < handler->menu_count; i++) {
            if (handler->menus[i]->active) {
                menu_handle_key_release(handler->menus[i], kr);

                // Check if modifiers released completely
                if ((kr->state & handler->menus[i]->active_modifier) == 0) {
                    handler->menus[i]->active = false;
                    printf("Menu %zu deactivated\n", i);
                }
            }
        }
        break;
    }

    default:
        printf("[DEBUG] Unhandled event type: 0x%x\n", event->response_type & ~0x80);
        break;
    }

    return exit;
}

InputHandler *input_handler_create(xcb_connection_t *conn,
                                   xcb_ewmh_connection_t *ewmh,
                                   xcb_window_t window) {
    if (!conn || !ewmh || window == XCB_NONE) {
        return NULL;
    }

    InputHandler *handler = calloc(1, sizeof(InputHandler));
    if (!handler) {
        return NULL;
    }

    handler->conn = conn;
    handler->focus_ctx = x11_focus_init(conn, ewmh);
    if (!handler->focus_ctx) {
        free(handler);
        return NULL;
    }

    x11_set_window_floating(handler->focus_ctx, window);
    if (!x11_grab_inputs(handler->focus_ctx, window)) {
        fprintf(stderr, "Unable to grab inputs\n");
        x11_focus_cleanup(handler->focus_ctx);
        free(handler);
        return NULL;
    }
    return handler;
}

void input_handler_destroy(InputHandler *handler) {
    if (!handler) {
        return;
    }
    if (handler->focus_ctx) {
        x11_release_inputs(handler->focus_ctx);
        x11_focus_cleanup(handler->focus_ctx);
    }
    free(handler);
}

bool input_handler_add_menu(InputHandler *handler, MenuConfig config) {
    if (!handler || !config.action_cb) {
        return false;
    }

    if (handler->menu_count >= MAX_MENUS) {
        fprintf(stderr, "Maximum menus reached\n");
        return false;
    }

    MenuContext *menu_ctx = menu_create(handler->focus_ctx, config);
    if (!menu_ctx) {
        return false;
    }

    handler->menus[handler->menu_count++] = menu_ctx;
    return true;
}

bool input_handler_process_event(InputHandler *handler) {
    xcb_generic_event_t *event = xcb_poll_for_event(handler->conn);
    if (!event) {
        return false;
    }

    bool should_exit = handle_event(handler, event);
    free(event);
    return should_exit;
}

void input_handler_run(InputHandler *handler) {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(handler->conn))) {
        if (handle_event(handler, event)) {
            free(event);
            break;
        }
        free(event);
    }
}
