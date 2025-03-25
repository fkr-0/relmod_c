#ifndef KEY_HELPER_H_
#define KEY_HELPER_H_

#include <xcb/xcb.h>

/* Key codes and masks + helpers */

#define SUPER_KEY 133
#define SUPER_MASK 0x40
#define CTRL_KEY 37
#define CTRL_MASK 0x04
#define ALT_KEY 64
#define ALT_MASK 0x08
#define SHIFT_KEY 50
#define SHIFT_MASK 0x01

int key_mod(int key);
int mod_key(int mask);

int mod_add(int state, int mod_key);
int mod_remove(int state, int mod_key);

uint16_t mod_state(xcb_connection_t *conn);
xcb_generic_event_t key_release(uint8_t keycode, uint16_t state);
xcb_generic_event_t key_press(uint8_t keycode, uint16_t state);

#endif // KEY_HELPER_H_
