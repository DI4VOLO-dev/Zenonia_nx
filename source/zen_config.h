/* zen_config.h -- the user's settings (<game folder>/config.ini). MIT. */
#ifndef ZEN_CONFIG_INI_H
#define ZEN_CONFIG_INI_H

typedef struct {
  int smooth;        /* [display] smooth: the game's "linear" texture filter (getGLOptionLinear) */
  int touch_buttons; /* [display] touch_buttons: the game's own on-screen D-pad and buttons */
  int volume;        /* [audio] volume, 0-100 */
  int swap_ab;       /* [controls] swap_a_b */
} ZenConfig;

const ZenConfig *zen_config(void);

#endif
