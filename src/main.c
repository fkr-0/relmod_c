/* ======= main.c ======= */
#include "cairo_menu.h"
#include "example_menu.h"
#include "input_handler.h"
#include "menu.h"
#include "version.h"
#include "window_menu.h"
#include "x11_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

int main(int argc, char *argv[]) {
    printf("X11 Window Manager v%s\n", VERSION);

    xcb_connection_t *conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) {
        fprintf(stderr, "Failed to connect to X server\n");
        return EXIT_FAILURE;
    }

    xcb_ewmh_connection_t ewmh = {0};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &ewmh);
    xcb_ewmh_init_atoms_replies(&ewmh, cookie, (void *)0);

    xcb_window_t root = xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;

    // Initialize window list
    WindowList *window_list = window_list_init(conn);
    if (!window_list) {
        fprintf(stderr, "Failed to initialize window list\n");
        xcb_disconnect(conn);
        return EXIT_FAILURE;
    }

    // Create input handler for window menu
    InputHandler *handler = input_handler_create(conn, &ewmh, root);
    if (!handler) {
        fprintf(stderr, "Failed to create input handler\n");
        window_list_free(window_list);
        xcb_disconnect(conn);
        return EXIT_FAILURE;
    }

    // Create window management menu
    MenuConfig window_menu = window_menu_create(conn, root, XCB_MOD_MASK_4, window_list);
    input_handler_add_menu(handler, window_menu);

    // Main event loop
    input_handler_run(handler);

    // Cleanup
    input_handler_destroy(handler);
    window_list_free(window_list);
    xcb_ewmh_connection_wipe(&ewmh);
    xcb_disconnect(conn);

    return EXIT_SUCCESS;
}
