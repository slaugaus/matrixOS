/********************************************************************************
                            __                __             ______    ______  
                           |  \              |  \           /      \  /      \ 
   ______ ____    ______  _| $$_     ______   \$$ __    __ |  $$$$$$\|  $$$$$$\
  |      \    \  |      \|   $$ \   /      \ |  \|  \  /  \| $$  | $$| $$___\$$
  | $$$$$$\$$$$\  \$$$$$$\\$$$$$$  |  $$$$$$\| $$ \$$\/  $$| $$  | $$ \$$    \ 
  | $$ | $$ | $$ /      $$ | $$ __ | $$   \$$| $$  >$$  $$ | $$  | $$ _\$$$$$$\
  | $$ | $$ | $$|  $$$$$$$ | $$|  \| $$      | $$ /  $$$$\ | $$__/ $$|  \__| $$
  | $$ | $$ | $$ \$$    $$  \$$  $$| $$      | $$|  $$ \$$\ \$$    $$ \$$    $$
  \ $$  \$$  \$$  \$$$$$$$   \$$$$  \$$       \$$ \$$   \$$  \$$$$$$   \$$$$$$ 
                                                                             
  by CJ Bialorucki, Jacob Deschamps, and Austin Slaughter
    (with a lot of help)
                                                                             
********************************************************************************/

// MATRIX GLOBAL STUFF
#include "USBHost_t36.h"
#include "KeyboardUtils.h"

#define TERM_CHAR_WIDTH 4
#define TERM_CHAR_HEIGHT 6

#include <MatrixHardware_Teensy4_ShieldV5.h>  // SmartLED Shield for Teensy 4 (V5)
#include <SmartMatrix.h>

#define COLOR_DEPTH 24  // Color depth (bits-per-pixel) of gfx layers
const uint16_t kMatrixWidth = 192;
const uint16_t kMatrixHeight = 128;
const uint8_t kRefreshDepth = 36;                               // Tradeoff of color quality vs refresh rate, max brightness, and RAM usage.  36 is typically good, drop down to 24 if you need to.  On Teensy, multiples of 3, up to 48: 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48.  On ESP32: 24, 36, 48
const uint8_t kDmaBufferRows = 4;                               // known working: 2-4, use 2 to save RAM, more to keep from dropping frames and automatically lowering refresh rate.  (This isn't used on ESP32, leave as default)
const uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;  // 64x64 "HUB75E" 5-address panels
// see docs for options: https://github.com/pixelmatix/SmartMatrix/wiki
const uint32_t kMatrixOptions = (SM_HUB75_OPTIONS_C_SHAPE_STACKING);  // aka "serpentine" - flip alternate rows to save cable length
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

uint8_t matrixBrightness = 255;
uint16_t termCursorX = 0;
uint16_t termCursorY = 0;

rgb24 termBgColor = { 0, 0, 0 };
rgb24 termTextColor = { 0, 255, 0 };

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

#ifdef USE_ADAFRUIT_GFX_LAYERS
// there's not enough allocated memory to hold the long strings used by this sketch by default, this increases the memory, but it may not be large enough
SMARTMATRIX_ALLOCATE_GFX_MONO_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, 6 * 1024, 1, COLOR_DEPTH, kScrollingLayerOptions);
#else
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
#endif

// KEYBOARD GLOBAL STUFF
USBHost myusb;
KeyboardController keyboard1(myusb);
// while not referenced anywhere, this is what actually does key decoding
USBHIDParser hid1(myusb);

// SDIO GLOBAL STUFF

void setup() {
  // wait 5 sec for Arduino Serial Monitor
  unsigned long start = millis();
  while (!Serial)
    if (millis() - start > 5000)
      break;

  Serial.println("\n\n### matrixOS Serial Console ###\n");
  // MATRIX INIT STUFF
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer);
  matrix.addLayer(&indexedLayer);
  matrix.begin();

  matrix.setBrightness(matrixBrightness);

  backgroundLayer.enableColorCorrection(true);  // probably good to have IDK
  backgroundLayer.setFont(font3x5);

  // KEYBOARD INIT STUFF
  myusb.begin();
  keyboard1.attachPress(OnPress);
  // keyboard1.attachRawPress(OnRawPress);
  // keyboard1.attachRawRelease(OnRawRelease);
  // SDIO INIT STUFF
}

void loop() {
  myusb.Task();
  // Everything else should be a TeensyThread... eventually
  // clear screen
  // backgroundLayer.fillScreen(defaultBackgroundColor);
  // backgroundLayer.swapBuffers();
}

// MATRIX FUNCTIONS
void advanceCursor(){
  termCursorX += TERM_CHAR_WIDTH;
  if (termCursorX > kMatrixWidth){
    termCursorX = 0;
    termCursorY += TERM_CHAR_HEIGHT;
  }
}

// KEYBOARD FUNCTIONS
// TODO: as this is (presumably) an ISR, consider limiting what it does
void OnPress(int key) {
  Serial.println("POLO");
  unsigned char key_dec = decodeKey(key, keyboard1.getOemKey(), keyboard1.getModifiers(), keyboard1.LEDS());
  if (key_dec) {
    Serial.print((char)key_dec);
    backgroundLayer.drawChar(termCursorX, termCursorY, termTextColor, (char)key_dec);
    advanceCursor();
    backgroundLayer.swapBuffers();
  }
}

// void OnRawPress(uint8_t keycode) {
//   Serial.print("OnRawPress keycode: ");
//   Serial.print(keycode, HEX);
//   Serial.print(" Modifiers: ");
//   Serial.println(keyboard_modifiers, HEX);
// }

// void OnRawRelease(uint8_t keycode) {
//   Serial.print("OnRawRelease keycode: ");
//   Serial.print(keycode, HEX);
//   Serial.print(" Modifiers: ");
//   Serial.println(keyboard1.getModifiers(), HEX);
// }

// SDIO FUNCTIONS
