// #include "USBHost_t36.h"
#include "KeyboardUtils.h"
#include <Arduino.h>
#include "usb_hid_keys.h"

#define KB_DEBUG_PRINT

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

// too lazy to make this a macro
bool isTypableKey(int oemKey, int leds) {
  return (oemKey >= KEY_A && oemKey <= KEY_ENTER) ||  // letters, num row, enter
         (oemKey >= KEY_TAB && oemKey <= KEY_SLASH) ||  // punctuation
         (oemKey >= KEY_KPSLASH && oemKey <= KEY_KPENTER) ||  // keypad punctuation
         (!(leds-1) && oemKey >= KEY_KP1 && oemKey <= KEY_KPDOT)  // keypad nums if NumLock
  ;
}

unsigned char decodeKey(int key, int oemKey, int mods, int leds){

  // Assumes mods is directly from .getModifiers()... harmless if it's not
  mods = combineMods(mods);

  // Only shift or nothing held - pass letters
  // if (!(mods-2) || !mods) {
  if ( !((mods-2) && mods) && isTypableKey(oemKey, leds) ) {  // De Morgan's Theorem
    return key;
  }
  // Otherwise it's a shortcut
  else {
    handleNonChar(oemKey, mods);
    return 0;
  }
}

void handleNonChar(int oemKey, int mods){
  #ifdef KB_DEBUG_PRINT
  Serial.print("Pressed shortcut or non-character - MODS[");
  Serial.print(mods, HEX);
  Serial.print("] + OEM[");
  Serial.print(oemKey, HEX);
  Serial.println("]");
  #endif
  // Assumes mods was ran through combineMods
  switch(mods){
    case Ctrl:
    // Shift caught in decodeKey for capitalization
    case CtrlShift:
    case Alt:
    case CtrlAlt:
    case AltShift:
    case CtrlAltShift:
      break;
    // Ignore Win key for now :)
  }
}

/* Important OEM codes
  See also usb_hid_keys.h
  A-Z: 4-1D
  1-0: 1E-27
  Num 1-0: 59-62 (LEDs 1 if using NumLock)
  F1-F12: 3A-45
  PrntSc-PgDn block L-R: 46-4E
  Enter: 28
  Esc: 29
  Backspace: 2A
  Tab: 2B

  Space -  =  [  ]  \  ;  '  `  ,  .  /  Caps
  2C    2D 2E 2F 30 31 33 34 35 36 37 38 39

  Arrows RLDU: 4F-52

  NumLk  /  *  -  +  Ent  .
  53     54 55 56 57 58   62

  Menu (Right Click): 65
 */