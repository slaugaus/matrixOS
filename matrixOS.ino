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

// #include <TeensyThreads.h>

// MATRIX GLOBAL STUFF
// font3x5, font5x7, font6x10, font8x13, gohufont11, gohufont11b
#define TERM_DEFAULT_FONT font3x5
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

uint16_t termLastLineCurX = 0;
uint16_t termLastLineCurY = 0;

const rgb24 colorWhite = { 255, 255, 255 };
const rgb24 colorBlack = { 0, 0, 0 };
const rgb24 colorRed = { 255, 0, 0 };
const rgb24 colorGreen = { 0, 255, 0 };
const rgb24 colorBlue = { 0, 0, 255 };

rgb24 termBgColor = colorBlack;
rgb24 termTextColor = colorGreen;

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

#ifdef USE_ADAFRUIT_GFX_LAYERS
// there's not enough allocated memory to hold the long strings used by this sketch by default, this increases the memory, but it may not be large enough
SMARTMATRIX_ALLOCATE_GFX_MONO_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, 6 * 1024, 1, COLOR_DEPTH, kScrollingLayerOptions);
#else
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
#endif

// KEYBOARD/CLI GLOBAL STUFF
#include "USBHost_t36.h"
#include "KeyboardUtils.h"

USBHost myusb;
KeyboardController keyboard1(myusb);
// while not referenced anywhere, this is what actually does key decoding
USBHIDParser hid1(myusb);

// TODO: Use this (should it be a std::string?)
char commandBuffer[128];
uint8_t commandBufferIdx = 0;
// Add typable keystrokes to it
// Read and flush on Enter press
// Prevent typing if full?

// SDIO GLOBAL STUFF

// MATRIX FUNCTIONS

// CLI FUNCTIONS
void cursorNewline() {
  // save last cursor pos for backspace
  termLastLineCurX = termCursorX;
  termLastLineCurY = termCursorY;

  termCursorX = 0;
  termCursorY += TERM_CHAR_HEIGHT;
}

// TODO: doesn't go up lines for some reason (cursor numbers wrong?)
void cursorBackspace() {
  termCursorX -= TERM_CHAR_WIDTH;
  if (termCursorX < 0) {
    if (termLastLineCurX || termLastLineCurY) {
      // restore last cursor pos
      termCursorX = termLastLineCurX;
      termCursorY = termLastLineCurY;
    }
    else{
      termCursorX = kMatrixWidth - TERM_CHAR_WIDTH;  // last character
      termCursorY -= TERM_CHAR_HEIGHT;
    }
  }
  // lol, lmao
  backgroundLayer.fillRectangle(termCursorX, termCursorY, termCursorX+TERM_CHAR_WIDTH, termCursorY+TERM_CHAR_HEIGHT, termBgColor);
}

void advanceCursor() {
  termCursorX += TERM_CHAR_WIDTH;
  if (termCursorX >= kMatrixWidth) cursorNewline();
}

// Advances the cursor in character units
void moveCursor(int dx = 0, int dy = 0) {
  termCursorX += (dx * TERM_CHAR_WIDTH);
  if (termCursorX >= kMatrixWidth) {
    termCursorX = 0;
    termCursorY += TERM_CHAR_HEIGHT;
  }
  termCursorY += (dy * TERM_CHAR_HEIGHT);
}

// Moves the cursor to an arbitrary position. Does nothing if you go offscreen
void setCursor(int x = -1, int y = -1) {
  if (x < kMatrixWidth && x >= 0) termCursorX = x;
  if (y < kMatrixHeight && x >= 0) termCursorY = y;
}

void bgDrawChar(rgb24 color, char chr) {
  backgroundLayer.drawChar(termCursorX, termCursorY, color, chr);
  advanceCursor();
}

void bgDrawString(rgb24 color, const char text[], bool newLine = true) {
  int offset = 0;
  char character;
  // iterate through string
  while ((character = text[offset++]) != '\0') {
    if (character == '\n') {
      cursorNewline();
      continue;
    }
    bgDrawChar(color, character);
  }
  if (newLine) cursorNewline();
}

// KEYBOARD FUNCTIONS
void routeKbSpecial(nonCharsAndShortcuts key){
  switch(key){
    case Enter: cursorNewline(); Serial.println(); break;
    case Backspace: cursorBackspace(); break;
    case Tab:
    case Esc: break;
  }
}

// TODO: as this is (presumably) an ISR, consider limiting what it does
void OnPress(int key) {
  unsigned char key_dec = decodeKey(key, keyboard1.getOemKey(), keyboard1.getModifiers(), keyboard1.LEDS());
  if (key_dec < 128) {
    Serial.print((char)key_dec);
    bgDrawChar(termTextColor, key_dec);
    commandBuffer[commandBufferIdx++] = key_dec;
  }
  else routeKbSpecial(key_dec);
}

// SDIO FUNCTIONS


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
  backgroundLayer.setFont(TERM_DEFAULT_FONT);
  bgDrawString(colorWhite, "matrixOS - (c)2024 Austin S., CJ B., Jacob D.\n> ", false);
  // backgroundLayer.drawString(0, TERM_CHAR_HEIGHT, colorWhite, "> ");
  // moveCursor(2, 1);  // advance to start line

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
  backgroundLayer.swapBuffers();
}