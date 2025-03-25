#include "key_helper.h"
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

int key_mod(int key) {
  switch (key) {
  case SUPER_KEY:
    return SUPER_MASK; // map super key to its mask
  case CTRL_KEY:
    return CTRL_MASK; // map ctrl key to its mask
  case ALT_KEY:
    return ALT_MASK; // map alt key to its mask
  case SHIFT_KEY:
    return SHIFT_MASK; // map shift key to its mask
  default:
    return 0; // return 0 if key is unrecognized
  }
}

// convert modifier mask (e.g. CTRL_MASK) back to its key identifier (e.g.
// CTRL_KEY)
int mod_key(int mask) {
  switch (mask) {
  case SUPER_MASK:
    return SUPER_KEY; // map super mask to its key
  case CTRL_MASK:
    return CTRL_KEY; // map ctrl mask to its key
  case ALT_MASK:
    return ALT_KEY; // map alt mask to its key
  case SHIFT_MASK:
    return SHIFT_KEY; // map shift mask to its key
  default:
    return 0; // return 0 if mask is unrecognized
  }
}

int mod_add(int state, int mod_key) { return state | mod_key; }
int mod_remove(int state, int mod_key) { return state & ~mod_key; }
xcb_generic_event_t key_press(uint8_t keycode, uint16_t state) {
  return *(xcb_generic_event_t *)&(
      (xcb_key_press_event_t){.response_type = XCB_KEY_PRESS,
                              .detail = keycode,
                              .state = state,
                              .time = XCB_CURRENT_TIME});
}

xcb_generic_event_t key_release(uint8_t keycode, uint16_t state) {
  return *(xcb_generic_event_t *)&(
      (xcb_key_release_event_t){.response_type = XCB_KEY_RELEASE,
                                .detail = keycode,
                                .state = state,
                                .time = XCB_CURRENT_TIME});
}

// Helper function to determine if a key is pressed
static int is_key_pressed(xcb_query_keymap_reply_t *reply, uint8_t keycode) {
  int byte = keycode / 8;
  int bit = keycode % 8;
  return (reply->keys[byte] & (1 << bit)) != 0;
}

// Main function to extract modifier state
uint16_t mod_state(xcb_connection_t *conn) {
  uint16_t state = 0;

  xcb_query_keymap_cookie_t cookie = xcb_query_keymap(conn);
  xcb_query_keymap_reply_t *reply = xcb_query_keymap_reply(conn, cookie, NULL);
  if (!reply) {
    // Error occurred; return empty state (0)
    return 0;
  }

  // Check Shift keys
  if (is_key_pressed(reply, SHIFT_KEY))
    state |= SHIFT_MASK;

  // Check Control keys
  if (is_key_pressed(reply, CTRL_KEY))
    state |= CTRL_MASK;

  // Check Alt keys (Mod1)
  if (is_key_pressed(reply, ALT_KEY))
    state |= ALT_MASK;

  // Check Super keys (Mod4)
  if (is_key_pressed(reply, SUPER_KEY))
    state |= SUPER_MASK;

  // Check Caps Lock state
  free(reply);
  return state;
}
