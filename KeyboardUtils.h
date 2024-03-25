/***********************************************************
* Utilities for decoding USB keyboard keys
***********************************************************/

#ifndef KEYBOARD_UTILS_H
#define KEYBOARD_UTILS_H

enum keyModCombos{
  Ctrl = 1,
  Shift,  // 2
  CtrlShift,
  Alt,  // 4
  CtrlAlt,
  AltShift,
  CtrlAltShift,
  Win,  // 8
  CtrlWin,
  ShiftWin,
  CtrlShiftWin,
  AltWin,
  CtrlAltWin,
  AltShiftWin,
  CtrlShiftAltWin  // 15
};

// sucky messaging system
enum nonCharsAndShortcuts{
  Esc = 128,
  Enter,
  Backspace,
  Tab
};

// Right modifier keys are on bits 7..4 - we don't care, so squish them together
#define combineMods(mods) ((mods & 0b1111) | (mods >> 4))

/* 
  Returns the keycode cast to a char, which generally matches the character you expect.
  For special keys, calls an appropriate function and returns 0 or something appropriate.
 */
unsigned char decodeKey(int key, int oemKey, int mods, int leds);

/*
  Called from decodeKey; maps OEM keycodes (constant regardless of modifiers) + modifiers to functions
  (Note that we only get 127 shortcuts - should be plenty though)
 */
unsigned char handleNonChar(int oemKey, int mods);

#endif