/***********************************************************
* Utilities for decoding USB keyboard keys
***********************************************************/

// Right modifier keys are on bits 7..4 - we don't care, so squish them together
#define combineMods(mods) ((mods & 0b1111) | (mods >> 4))

/* 
  Returns the keycode cast to a char, which generally matches the character you expect.
  For special keys, calls an appropriate function and returns 0 or something appropriate.
 */
unsigned char decodeKey(int key, int oemKey, int mods, int leds);

/*
  Called from decodeKey; maps OEM keycodes (constant regardless of modifiers) + modifiers to functions
 */
void handleNonChar(int oemKey, int mods);