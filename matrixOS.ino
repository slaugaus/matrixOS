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

#include <TeensyThreads.h>
#include <string.h>
#include <stdio.h>

#define flushString(str) memset(str, 0, strlen(str))

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
rgb24 termInputColor = colorGreen;
rgb24 termResponseColor = colorWhite;

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
#include "HelpStrings.h"

// # of arguments (including command) allowed in a CLI command
#define COMMAND_TOKEN_LIMIT 8
#define COMMAND_MAX_LENGTH 128

#define PROMPT "> "
#define DRAW_PROMPT bgDrawString(termResponseColor, PROMPT, false)

USBHost myusb;
KeyboardController keyboard1(myusb);
// while not referenced anywhere, this is what actually does key decoding
USBHIDParser hid1(myusb);

// TODO: Use this (should it be a std::string?)
char commandBuffer[COMMAND_MAX_LENGTH];
uint8_t commandBufferIdx = 0;
char serialCommandBuffer[COMMAND_MAX_LENGTH];
uint8_t serialCommandBufferIdx = 0;
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

  Serial.println();
}

// TODO: doesn't go up lines for some reason (cursor numbers wrong?)
// TODO TOO: commandBuffer[i--] = \0 and redraw the string instead
void cursorBackspace() {
  termCursorX -= TERM_CHAR_WIDTH;
  if (termCursorX < 0) {
    if (termLastLineCurX || termLastLineCurY) {
      // restore last cursor pos
      termCursorX = termLastLineCurX;
      termCursorY = termLastLineCurY;
    } else {
      termCursorX = kMatrixWidth - TERM_CHAR_WIDTH;  // last character
      termCursorY -= TERM_CHAR_HEIGHT;
    }
  }
  // lol, lmao
  backgroundLayer.fillRectangle(termCursorX, termCursorY, termCursorX + TERM_CHAR_WIDTH, termCursorY + TERM_CHAR_HEIGHT, termBgColor);
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

// Moves the cursor to an arbitrary position. Does nothing if you try to go offscreen
void setCursor(int x = -1, int y = -1) {
  if (x < kMatrixWidth && x >= 0) termCursorX = x;
  if (y < kMatrixHeight && x >= 0) termCursorY = y;
}

// TODO: Default color (need to reorder all calls)
void bgDrawChar(rgb24 color, char chr) {
  backgroundLayer.drawChar(termCursorX, termCursorY, color, chr);
  Serial.print(chr);
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

// --- CLI COMMANDS ---
// Split a command string by spaces, then call the appropriate function
void parseCommand(char text[]) {
  Serial.println("Parse begin");

  if (!text[0]){
    Serial.println("Empty string :(");
    return;
  }
  cursorNewline();
  int i = 0;
  char *tokens[COMMAND_TOKEN_LIMIT];       // array of strings, kind of
  tokens[0] = strtok(text, " ");  // command name should be case-insensitive
  while (tokens[i++] != NULL)
    tokens[i] = strtok(NULL, " ");

  Serial.print(tokens[0]);
  Serial.println(" is tokens 0");

  if (!strcmp(tokens[0], "help")) help(tokens);
  else invalidCommand(tokens[0]);

  DRAW_PROMPT;
  Serial.println("Parse end");
}

void help(char *tokens[]) {
  Serial.println("Help");
  if (!tokens[1]) {
    bgDrawString(termResponseColor, COMMANDS_AVAILABLE);
  } else if (!strcmp(tokens[1], "help")) {
    bgDrawString(colorRed, "Oh you think you're funny do ya?");
  } else {
    invalidCommand(tokens[1]);
  }
}

//"token" is not a valid command.\nCOMMANDS_AVAILABLE
void invalidCommand(char token[]) {
  Serial.println("Invalid");
  bgDrawChar(termResponseColor, '\"');
  bgDrawString(termResponseColor, token, false);
  bgDrawString(termResponseColor, "\" is not a valid command.");
  bgDrawString(termResponseColor, COMMANDS_AVAILABLE);
}
// ---END CLI COMMANDS ---

// KEYBOARD FUNCTIONS
void routeKbSpecial(nonCharsAndShortcuts key) {
  Serial.println("Route");
  switch (key) {
    // case Enter: cursorNewline(); Serial.println(); break;
    case Enter:
      // threads.addThread(parseCommand, commandBuffer);
      parseCommand(commandBuffer);
      Serial.println("Flush buffer");
      flushString(commandBuffer);
      commandBufferIdx = 0;
      break;
    case Backspace: cursorBackspace(); break;
    case Tab:
    case Esc: break;
  }
}

// TODO: as this is (presumably) an ISR, consider limiting what it does
void OnPress(int key) {
  unsigned char key_dec = decodeKey(key, keyboard1.getOemKey(), keyboard1.getModifiers(), keyboard1.LEDS());
  if (key_dec < 128) {
    // Serial.print((char)key_dec);
    bgDrawChar(termInputColor, key_dec);
    commandBuffer[commandBufferIdx++] = key_dec;
  } else routeKbSpecial(key_dec);
}

// SDIO FUNCTIONS


void setup() {
  // wait 5 sec for Arduino Serial Monitor
  Serial.begin(9600);
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
  bgDrawString(termResponseColor, "matrixOS - (c)2024 Austin S., CJ B., Jacob D.");
  bgDrawString(termResponseColor, PROMPT, false);
  // backgroundLayer.drawString(0, TERM_CHAR_HEIGHT, termResponseColor, "> ");
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

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.

  Serial inputs will be parsed as commands.
*/
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    if (inChar == '\n' && serialCommandBufferIdx) {
      bgDrawString(colorBlue, "Serial: ", false);
      bgDrawString(colorBlue, serialCommandBuffer, false);
      parseCommand(serialCommandBuffer);
      flushString(serialCommandBuffer);
      serialCommandBufferIdx = 0;
    } else {
      // add it to the inputString:
      serialCommandBuffer[serialCommandBufferIdx++] = inChar;
    }
    Serial.print("\ninputString is ");
    Serial.println(serialCommandBuffer);
  }
}