// #include "USBHost_t36.h"
#include "KeyboardUtils.h"
#include <Arduino.h>
// #include <avr/wdt.h>
#include "usb_hid_keys.h"
// #include "signals.h"

// #define KB_DEBUG_PRINT

// too lazy to make this a macro
bool isTypableKey(int oemKey, int leds) {
  return (oemKey >= KEY_A && oemKey <= KEY_0) ||  // letters, num row
         (oemKey >= KEY_MINUS && oemKey <= KEY_SLASH) ||  // punctuation
         (oemKey >= KEY_KPSLASH && oemKey <= KEY_KPPLUS) ||  // keypad punctuation
         (!(leds-1) && oemKey >= KEY_KP1 && oemKey <= KEY_KPDOT)  // keypad nums if NumLock
  ;
}

unsigned char decodeKey(int key, int oemKey, int mods, int leds){

  // Assumes mods is directly from .getModifiers()... harmless if it's not
  mods = combineMods(mods);

  // Only shift or nothing held - pass letters
  // if (!(mods-2) || !mods) {
  if ( !((mods-2) && mods) && isTypableKey(oemKey, leds)) {  // De Morgan's Theorem
    return key;
  }
  // Otherwise it's a shortcut
  else {
    return handleNonChar(oemKey, mods);
  }
}

unsigned char handleNonChar(int oemKey, int mods){
  #ifdef KB_DEBUG_PRINT
  Serial.println();
  Serial.print("Pressed shortcut or non-character - MODS[");
  Serial.print(mods, HEX);
  Serial.print("] + OEM[");
  Serial.print(oemKey, HEX);
  Serial.println("]");
  #endif
  // Assumes mods was ran through combineMods
  switch(mods){
    case Ctrl:
      switch(oemKey){
        case KEY_L: return CtrlL; // Ctrl+L clears screen
        case KEY_C: return Esc;  // Ctrl + C escapes for cancel
      }
    // Shift caught in decodeKey for capitalization
    case CtrlShift:
    case Alt: break;
    case CtrlAlt:
      switch(oemKey){
        case KEY_DELETE: return CtrlAltDelete;
      }
    case AltShift:
    case CtrlAltShift:
      break;
    // Ignore Win key for now :)
    default:
      switch(oemKey){
        case KEY_SPACE: return ' '; // Shift+Space returns a non-char; stupid hack to not do that
        case KEY_ENTER: case KEY_KPENTER: return Enter;
        case KEY_BACKSPACE: return Backspace;
        case KEY_TAB: return Tab;
        case KEY_ESC: return Esc;
        case KEY_LEFT: return Left;
        case KEY_RIGHT: return Right;
        case KEY_F5: return F5;
        case KEY_F6: return F6;
      }
  }
  return 0;
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