#ifndef KEY_HELPER_H_
#define KEY_HELPER_H_

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

#endif // KEY_HELPER_H_
