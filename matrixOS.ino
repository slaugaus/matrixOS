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
#include "screen.h"
#include "CommandTable.h"
// KEYBOARD/CLI GLOBAL STUFF
#include "USBHost_t36.h"
#include "KeyboardUtils.h"
#include "HelpStrings.h"
// SDIO GLOBAL STUFF
#include <SD.h>
#include <GifDecoder.h>
#include "FilenameFunctions.h"

// # of arguments (including command) allowed in a CLI command
#define COMMAND_TOKEN_LIMIT 8
#define COMMAND_MAX_LENGTH 128
#define PROMPT "> "
#define DRAW_PROMPT bgDrawString(MainScreen->termResponseColor, PROMPT, false)
// Chip select for SD card
#define SD_CS BUILTIN_SDCARD
// Teensy SD Library requires a trailing slash in the directory name
#define GIF_DIRECTORY "/gif/"
#define flushString(str) memset(str, 0, strlen(str))

// set false if another app needs buffer control
bool inCLI = true;

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
// TODO: Can't have 2 of these... probably need an AdafruitGFX bg layer?
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(gfxLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
// SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

// #ifdef USE_ADAFRUIT_GFX_LAYERS
// // there's not enough allocated memory to hold the long strings used by this sketch by default, this increases the memory, but it may not be large enough
// SMARTMATRIX_ALLOCATE_GFX_MONO_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, 6 * 1024, 1, COLOR_DEPTH, kScrollingLayerOptions);
// #else
// SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);
// #endif

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

/* template parameters are maxGifWidth, maxGifHeight, lzwMaxBits
 * 
 * lzwMaxBits is included for backwards compatibility reasons, but isn't used anymore
 */
GifDecoder<kMatrixWidth, kMatrixHeight, 12> decoder;

Screen *MainScreen = new Screen();

// MATRIX FUNCTIONS

// CLI FUNCTIONS
void cursorNewline() {
  // save last cursor pos for backspace
  MainScreen->termLastLineCurX = MainScreen->termCursorX;
  MainScreen->termLastLineCurY = MainScreen->termCursorY;

  MainScreen->termCursorX = 0;
  MainScreen->termCursorY += CHAR_HEIGHT;

  Serial.println();
}

// TODO: doesn't go up lines for some reason (cursor numbers wrong?)
// TODO TOO: commandBuffer[i--] = \0 and redraw the string instead
void cursorBackspace() {
  MainScreen->termCursorX -= CHAR_WIDTH;
  if (MainScreen->termCursorX < 0) {
    if (MainScreen->termLastLineCurX || MainScreen->termLastLineCurY) {
      // restore last cursor pos
      MainScreen->termCursorX = MainScreen->termLastLineCurX;
      MainScreen->termCursorY = MainScreen->termLastLineCurY;
    } else {
      MainScreen->termCursorX = kMatrixWidth - CHAR_WIDTH;  // last character
      MainScreen->termCursorY -= CHAR_HEIGHT;
    }
  }
  // lol, lmao even
  backgroundLayer.fillRectangle(MainScreen->termCursorX, MainScreen->termCursorY, MainScreen->termCursorX + CHAR_WIDTH, MainScreen->termCursorY + CHAR_HEIGHT, MainScreen->termBgColor);
}

void advanceCursor() {
  MainScreen->termCursorX += CHAR_WIDTH;
  if (MainScreen->termCursorX >= kMatrixWidth) cursorNewline();
}

// Advances the cursor in character units
void moveCursor(int dx = 0, int dy = 0) {
  MainScreen->termCursorX += (dx * CHAR_WIDTH);
  if (MainScreen->termCursorX >= kMatrixWidth) {
    MainScreen->termCursorX = 0;
    MainScreen->termCursorY += CHAR_HEIGHT;
  }
  MainScreen->termCursorY += (dy * CHAR_HEIGHT);
}

// Moves the cursor to an arbitrary position. Does nothing if you try to go offscreen
void setCursor(int x = -1, int y = -1) {
  if (x < kMatrixWidth && x >= 0) MainScreen->termCursorX = x;
  if (y < kMatrixHeight && x >= 0) MainScreen->termCursorY = y;
}

// TODO: Default color (need to reorder all calls)
void bgDrawChar(rgb24 color, char chr) {
  backgroundLayer.drawChar(MainScreen->termCursorX, MainScreen->termCursorY, color, chr);
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

  if (!text[0]) {
    Serial.println("Empty string :(");
    return;
  }
  cursorNewline();
  int i = 0;
  char *tokens[COMMAND_TOKEN_LIMIT];  // array of strings, kind of
  tokens[0] = strtok(text, " ");      // command name should be case-insensitive
  while (tokens[i++] != NULL)
    tokens[i] = strtok(NULL, " ");

  Serial.print(tokens[0]);
  Serial.println(" is tokens 0");

  Command * command = getCommand(tokens[0]);

  if (command){
    command->function(tokens);
  }
  else{
    invalidCommand(tokens[0]);
  }


  DRAW_PROMPT;
  Serial.println("Parse end");
}

int help(void * args) {
  char **tokens = (char**) args;
  Serial.println("Help");
  if (!tokens[1]) {
    bgDrawString(MainScreen->termResponseColor, COMMANDS_AVAILABLE);
  } else if (!strcmp(tokens[1], "help")) {
    bgDrawString(MainScreen->termErrorColor, "Oh you think you're funny do ya?");
  } else {
    invalidCommand(tokens[1]);
  }
  return 0;
}

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, bool displayFilenames) {
  numberOfFiles = 0;
  File directory = SD.open(directoryName);
  if (!directory) {
    return -1;
  }
  while (file = directory.openNextFile()) {
    if (isAnimationFile(file.name())) {
      numberOfFiles++;
      if (displayFilenames) {
        char toDisplay[64];
        snprintf(toDisplay, 63, "%d: %s", numberOfFiles, file.name());
        bgDrawString(MainScreen->termResponseColor, toDisplay);
      }
    } else if (displayFilenames) {
      bgDrawString(MainScreen->termResponseColor, "Non-GIF: ", false);
      bgDrawString(MainScreen->termResponseColor, file.name());
    }
    file.close();
  }
  //    file.close();
  directory.close();
  return numberOfFiles;
}

int cliGif(void* args) {
  char **tokens = (char**) args;
  int num_files = enumerateGIFFiles(GIF_DIRECTORY, true);
  if (num_files < 0) {
    bgDrawString(MainScreen->termErrorColor, "No gif directory on SD card.");
    return 1;
  }
  if (tokens[1]) {
    int idx = atoi(tokens[1]);
    if (idx > num_files || idx < 1) {
      bgDrawString(MainScreen->termErrorColor, "Invalid file number.");
      return 1;
    }
    inCLI = false;
    // threads.addThread(gifPlayerLoop, idx);
    gifPlayerLoop(idx);
  }
  return 0;
}

//"token" is not a valid command.\nCOMMANDS_AVAILABLE
void invalidCommand(char token[]) {
  Serial.println("Invalid");
  bgDrawChar(MainScreen->termResponseColor, '\"');
  bgDrawString(MainScreen->termResponseColor, token, false);
  bgDrawString(MainScreen->termResponseColor, "\" is not a valid command.");
  bgDrawString(MainScreen->termResponseColor, COMMANDS_AVAILABLE);
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
  if (key_dec && key_dec < 128) {
    // Serial.print((char)key_dec);
    bgDrawChar(MainScreen->termInputColor, key_dec);
    commandBuffer[commandBufferIdx++] = key_dec;
  } else routeKbSpecial(key_dec);
}

// SDIO FUNCTIONS
void screenClearCallback(void) {
  gfxLayer.fillScreen(colorBlack);
}

void updateScreenCallback(void) {
  gfxLayer.swapBuffers();
  // return;
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  gfxLayer.drawPixel(x, y, { red, green, blue });
}

// APP THREADS
void gifPlayerLoop(int index) {
  inCLI = false;
  Serial.println("GIF Loop Entered");
  static unsigned long displayStartTime_millis;
  unsigned long now = millis();
  // matrix.addLayer(&gfxLayer);
  
  if (openGifFilenameByIndex(GIF_DIRECTORY, index) >= 0) {
    if (decoder.startDecoding() < 0) {
      Serial.println("Decoder broke for some reason");
      return;
    }
    displayStartTime_millis = now;
  }

  while (1) {  // TODO: Exit after finished playing or loop
    // Decoder uses callbacks to draw the GIF
    if (decoder.decodeFrame() < 0) {
      // There's an error with this GIF, go to the next one
      Serial.println("GIF Problem");
      return;
    }
  }
}

void setup() {
  // wait 5 sec for Arduino Serial Monitor
  // Serial.begin();
  unsigned long start = millis();
  while (!Serial)
    if (millis() - start > 5000)
      break;

  Serial.println("\n\n### matrixOS Serial Console ###\n");
  // MATRIX INIT STUFF
  // matrix.addLayer(&gfxLayer);
  matrix.addLayer(&backgroundLayer);
  // matrix.addLayer(&scrollingLayer);
  // matrix.addLayer(&indexedLayer);
  matrix.begin();

  matrix.setBrightness(MainScreen->matrixBrightness);

  backgroundLayer.enableColorCorrection(true);  // probably good to have IDK
  backgroundLayer.setFont(DEFAULT_FONT);

  // Clear screen
  // backgroundLayer.fillScreen(colorBlack);
  // backgroundLayer.swapBuffers();

  bgDrawString(MainScreen->termResponseColor, "matrixOS [Version 1.0.0.0]");
  bgDrawString(MainScreen->termResponseColor, "(c) 2024 Austin S., CJ B., Jacob D.");
  cursorNewline();
  bgDrawString(MainScreen->termResponseColor, PROMPT, false);
  // backgroundLayer.drawString(0, CHAR_HEIGHT, MainScreen->termResponseColor, "> ");
  // moveCursor(2, 1);  // advance to start line

  // KEYBOARD INIT STUFF
  myusb.begin();
  keyboard1.attachPress(OnPress);
  // keyboard1.attachRawPress(OnRawPress);
  // keyboard1.attachRawRelease(OnRawRelease);
  // SDIO INIT STUFF
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);
  // NOTE: new callback function required after we moved to using the external AnimatedGIF library to decode GIFs
  decoder.setFileSizeCallback(fileSizeCallback);


  if (!initCMDTable(100)){
    bgDrawString(colorRed, "Command Table Initialization Failed");
  }

  appendCommand("help", "Displays Help Screen", help);
  appendCommand("gif", "", cliGif);


  if (initFileSystem(SD_CS) < 0) {
    bgDrawString(colorRed, "No SD card, expect some apps to break");
    // Serial.println("No SD card, expect some apps to break");
  }
}

void loop() {
  myusb.Task();
  // Everything else should be a TeensyThread... eventually
  // clear screen
  // backgroundLayer.fillScreen(defaultBackgroundColor);
  // TODO: For some reason, GIF playback ONLY works if this line doesn't exist
  if (inCLI == true)
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
  }
}