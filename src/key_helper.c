#include "key_helper.h"
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
