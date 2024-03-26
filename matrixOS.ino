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
#include "signals.h"
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
#define DRAW_PROMPT cliDrawString(PROMPT, false)
// Chip select for SD card
#define SD_CS BUILTIN_SDCARD
// Teensy SD Library requires a trailing slash in the directory name
#define GIF_DIRECTORY "/gif/"
#define flushString(str) memset(str, 0, COMMAND_MAX_LENGTH)

// set false if another app needs buffer control
bool inCLI = true;

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
// CLI text (single color) goes on an indexed layer. Anything in "graphics mode" (images for now) goes on a full-color background layer.
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(gfxLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(textLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

USBHost myusb;
KeyboardController keyboard1(myusb);
// while not referenced anywhere, this is what actually does key decoding
USBHIDParser hid1(myusb);

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

// FUNCTION PROTOTYPES
void cursorNewline();
void cursorBackspace();
void advanceCursor();
void moveCursor(int dx, int dy);
void setCursor(int x, int y);
void cliDrawChar(char chr);
void cliDrawString(const char* text, bool newLine = true);
void parseCommand(char text[]);
void help(char *tokens[]);
int enumerateGIFFiles(const char *directoryName, bool displayFilenames);
void cliGif(char *tokens[]);
void invalidCommand(char token[]);
void routeKbSpecial(nonCharsAndShortcuts key);
void OnPress(int key);
void screenClearCallback(void);
void updateScreenCallback(void);
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);
void gifPlayerLoop(int index);

void setupCommands(void){
  if (!initCMDTable(100)){
    cliDrawString("Command Table Initialization Failed");
  }

  appendCommand("help", "Displays Help Screen", help);
  appendCommand("gif", "Plays a .gif file from the SD card", cliGif);
  appendCommand("echo", "*Echoes input to console", NULL);
  appendCommand("color", "*changes text color", NULL); // TODO: call textLayer.setIndexedColor
  appendCommand("cls", "*clears terminal output", NULL); 

  return;
}


void setup() {
  // wait 1 sec for Arduino Serial Monitor
  // Serial.begin();
  unsigned long start = millis();
  while (!Serial)
    if (millis() - start > 1000)
      break;

  Serial.println("\n\n### matrixOS Serial Console ###\n");
  // MATRIX INIT STUFF
  matrix.addLayer(&gfxLayer);
  matrix.addLayer(&textLayer);
  matrix.begin();

  matrix.setBrightness(MainScreen->matrixBrightness);

  gfxLayer.enableColorCorrection(true);
  textLayer.enableColorCorrection(true);
  textLayer.setFont(DEFAULT_FONT);

  // Clear screen
  // backgroundLayer.fillScreen(colorBlack);
  // backgroundLayer.swapBuffers();

  cliDrawString("matrixOS [Version 1.0.0.0]");
  cliDrawString("(c) 2024 Austin S., CJ B., Jacob D.");
  cursorNewline();

  // KEYBOARD INIT STUFF
  myusb.begin();
  keyboard1.attachPress(OnPress);
  // keyboard1.attachRawPress(OnRawPress);
  // keyboard1.attachRawRelease(OnRawRelease);
  // SDIO/GIF INIT STUFF
  decoder.setScreenClearCallback(screenClearCallback);
  decoder.setUpdateScreenCallback(updateScreenCallback);
  decoder.setDrawPixelCallback(drawPixelCallback);

  decoder.setFileSeekCallback(fileSeekCallback);
  decoder.setFilePositionCallback(filePositionCallback);
  decoder.setFileReadCallback(fileReadCallback);
  decoder.setFileReadBlockCallback(fileReadBlockCallback);
  decoder.setFileSizeCallback(fileSizeCallback);
  
  setupCommands();

  if (initFileSystem(SD_CS) < 0) {
    cliDrawString("No SD card, expect some apps to break. Try restarting.");
  }

  DRAW_PROMPT;
}

void loop() {
  myusb.Task();
  // Everything else should be a TeensyThread... eventually
  // clear screen
  // backgroundLayer.fillScreen(defaultBackgroundColor);
  
  if (isCommandAvailable()){
      parseCommand(commandBuffer);
      Serial.println("Flush buffer");
      flushString(commandBuffer);
      commandBufferIdx = 0;
  }

  if (inCLI == true)
    textLayer.swapBuffers();
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
    // enter was pressed with a command entered, parse it
    if (inChar == '\n' && serialCommandBufferIdx) {
      cliDrawString("Serial: ", false);
      cliDrawString(serialCommandBuffer, false);
      parseCommand(serialCommandBuffer);
      flushString(serialCommandBuffer);
      serialCommandBufferIdx = 0;
    } else {
      // add it to the inputString:
      serialCommandBuffer[serialCommandBufferIdx++] = inChar;
    }
  }
}

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

// TODO: hacky solution, do something with the command buffer instead
void cursorBackspace() {

  commandBuffer[commandBufferIdx--] = '\0';

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
  // TODO: indexed layers don't have this function
  // textLayer.fillRectangle(MainScreen->termCursorX, MainScreen->termCursorY, MainScreen->termCursorX + CHAR_WIDTH, MainScreen->termCursorY + CHAR_HEIGHT, MainScreen->termBgColor);
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

void cliDrawChar(char chr) {
  // 3rd argument (index) is unused (TODOd) in SmartMatrix
  textLayer.drawChar(MainScreen->termCursorX, MainScreen->termCursorY, 1, chr);
  Serial.print(chr);
  advanceCursor();
}

void cliDrawString(const char* text, bool newLine = true) {
  int offset = 0;
  char character;
  // iterate through string
  while ((character = text[offset++]) != '\0') {
    if (character == '\n') {
      cursorNewline();
      continue;
    }
    cliDrawChar(character);
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
  tokens[0] = strtok(text, " ");
  while (tokens[i++] != NULL)
    tokens[i] = strtok(NULL, " ");

  Serial.print(tokens[0]);
  Serial.println(" is tokens 0");

  Command * command = getCommand(tokens[0]);

  Serial.println("Command got");

  if (command){
    Serial.println("Command exists");
    if (command->function){
      command->function(tokens);
    }
    else{
      Serial.println("Command exists, but has no function");
      cliDrawString("Command exists, but has no function");
    }
  }
  else{
    Serial.println("Command not found");
    invalidCommand(tokens[0]);
  }


  DRAW_PROMPT;
  Serial.println("Parse end");
}

void DisplayHelp(Command * command){
  cliDrawString(command->title, false);
  cliDrawString(" - ", false);
  cliDrawString(command->helpInfo);
}

void ListCommands(void){
  unsigned long size = getTableSize();
  Command ** table = getTable();
  Command * cursor = NULL;
  for (unsigned long i = 0; i < size; i++){
    if (table[i] != NULL){
      cursor = table[i];
      DisplayHelp(cursor);
      while (cursor->next){
        cursor = cursor->next;
        DisplayHelp(cursor);
      }
    }
  }
}

int help(void * args) {
  char **tokens = (char**) args;
  Serial.println("Help entered");
  if (!tokens[1]) {
    // cliDrawString(COMMANDS_AVAILABLE);
    ListCommands();
  } else if (!strcmp(tokens[1], "help")) {
    cliDrawString("Oh you think you're funny do ya?");
  } else {
    Command * cmd = getCommand(tokens[1]);
    if (cmd){
      DisplayHelp(cmd);
    }
    else
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
      if (displayFilenames) {
        char toDisplay[64];
        snprintf(toDisplay, 63, "%d: %s", numberOfFiles + 1, file.name());
        cliDrawString(toDisplay);
      }
      numberOfFiles++;
    } else if (displayFilenames) {
      cliDrawString("Non-GIF: ", false);
      cliDrawString(file.name());
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
    cliDrawString("No gif directory on SD card.");
    return 1;
  }
  if (tokens[1]) {
    int idx = atoi(tokens[1]);
    if (idx > num_files || idx <= 0) {
      cliDrawString("Invalid file number.");
      return 1;
    }
    inCLI = false;
    // NOTE: This clears textLayer.
    // There doesn't seem to be a way to temporarily hide it.
    textLayer.fillScreen(0);
    textLayer.swapBuffers();
    // threads.addThread(gifPlayerLoop, idx);
    gifPlayerLoop(--idx);
  }
  return 0;
}

//"token" is not a valid command.\nCOMMANDS_AVAILABLE
void invalidCommand(char token[]) {
  // Serial.println("Invalid");
  cliDrawChar('\"');
  cliDrawString(token, false);
  cliDrawString("\" is not a valid command.");
  cliDrawString(COMMANDS_AVAILABLE);
}
// ---END CLI COMMANDS ---

// KEYBOARD FUNCTIONS
void routeKbSpecial(nonCharsAndShortcuts key) {
  // Serial.println("Special key");
  switch (key) {
    // case Enter: cursorNewline(); Serial.println(); break;
    case Enter:
      raiseCommandFlag();
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
    cliDrawChar(key_dec);
    commandBuffer[commandBufferIdx++] = key_dec;
  } else routeKbSpecial((nonCharsAndShortcuts) key_dec);
}

// SDIO FUNCTIONS
void screenClearCallback(void) {
  gfxLayer.fillScreen(colorBlack);
}

void updateScreenCallback(void) {
  gfxLayer.swapBuffers();
  // gfxLayer.swapBuffers();
  // // return;
}
void updateScreenTransparentCallback(void) {
  gfxLayer.swapBuffers();
  gfxLayer.fillScreen(colorBlack);
  // gfxLayer.swapBuffers();
  // // return;
}

void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  gfxLayer.drawPixel(x, y, { red, green, blue });
}

// APP THREADS
void gifPlayerLoop(int index) {
  // inCLI = false;
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