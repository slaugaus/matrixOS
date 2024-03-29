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
#include <string.h>
#include <stdio.h>
#include "screen.h"
#include "CommandTable.h"
#include "signals.h"
#include "getColor.h"
// KEYBOARD/CLI GLOBAL STUFF
#include <USBHost_t36.h>
#include "KeyboardUtils.h"
#include "HelpStrings.h"
#include "rainbow.h"
// SDIO GLOBAL STUFF
#include <SD.h>
#include <PNGdec.h>
#include <GifDecoder.h>
#include "FilenameFunctions.h"

// # of arguments (including command) allowed in a CLI command
#define COMMAND_TOKEN_LIMIT 8
#define COMMAND_MAX_LENGTH 128
#define PROMPT "> "
// Chip select for SD card
#define SD_CS BUILTIN_SDCARD
// Teensy SD Library requires a trailing slash in the directory name
#define GIF_DIRECTORY "/gif/"
#define PNG_DIRECTORY "/png/"
#define JPEG_DIRECTORY "/jpg/"

#define drawPrompt() cliDrawString(PROMPT, false)
#define flushString(str) memset(str, 0, COMMAND_MAX_LENGTH)

// set false if another app needs buffer control?
// bool inCLI = true;
bool runRGB = false;
int rgbIdx = 0;

// Processed macros to make a refactor easier, maybe
// SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
static volatile __attribute__ ((section(".dmabuffers"), used)) SmartMatrixRefreshT4<kRefreshDepth, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions>::rowDataStruct rowsDataBuffer[kDmaBufferRows];
SmartMatrixRefreshT4<kRefreshDepth, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrixRefresh(kDmaBufferRows, rowsDataBuffer);
SmartMatrixHub75Calc<kRefreshDepth, kMatrixWidth, kMatrixHeight, kPanelType, kMatrixOptions> matrix(kDmaBufferRows, rowsDataBuffer);

// CLI text (single color) goes on an indexed layer. Anything in "graphics mode" (images for now) goes on a full-color background layer.

// SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(gfxLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
typedef rgb24 SM_RGB;
static rgb24 gfxLayerBitmap[2*kMatrixWidth*kMatrixHeight];
static uint16_t gfxLayercolorCorrectionLUT[sizeof(SM_RGB) <= 3 ? 256 : 4096];
static SMLayerBackground<rgb24, kBackgroundLayerOptions> gfxLayer(gfxLayerBitmap, kMatrixWidth, kMatrixHeight, gfxLayercolorCorrectionLUT);

// SMARTMATRIX_ALLOCATE_INDEXED_LAYER(textLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);
typedef rgb24 SM_RGB;
static uint8_t textLayerBitmap[2 * kMatrixWidth * (kMatrixHeight / 8)];
static SMLayerIndexed<rgb24, kIndexedLayerOptions> textLayer(textLayerBitmap, kMatrixWidth, kMatrixHeight);

// Global colors for foreground and background
static rgb24 fcolor2 =  {255, 255, 255};
static rgb24 bcolor2  = {0, 0, 0};

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

PNG png;

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
int enumerateFiles(const char *directoryName, bool displayFilenames);
void invalidCommand(char token[]);
void routeKbSpecial(nonCharsAndShortcuts key);
void OnPress(int key);
void screenClearCallback(void);
void updateScreenCallback(void);
void drawPixelCallback(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue);
void gifPlayerLoop(int index);
void rgbTask();
int help(void * args);
int cliGif(void * args);
int echoText(void * args);
int displayVersion(void * args);
int clearScreen(void * args);
int setColor(void * args);
int cpuRestart(void * args);
int bright(void * args);
int cliRGB(void * args);

void setupCommands(void){
  if (!initCMDTable(100)){
    cliDrawString("Command Table Initialization Failed");
  }

  appendCommand("help", "Displays Help Screen", help);
  appendCommand("gif", "Plays a .gif file from the SD card", cliGif);
  appendCommand("echo", "Echoes input to console", echoText);
  appendCommand("color", "changes text & bg colors (0 - f)", setColor);
  appendCommand("cls", "clears terminal output (Ctrl+L)", clearScreen); 
  appendCommand("ver", "displays current version", displayVersion);
  appendCommand("reset", "resets the system (Ctrl+Alt+Del)", cpuRestart);
  appendCommand("bright", "changes the global brightness (10-255)", bright);
  appendCommand("rgb", "toggles RGB text mode", cliRGB);

}

void setup() {
  // wait 1 sec for Arduino Serial Monitor
  // Serial.begin();
  // unsigned long start = millis();
  while (!Serial && millis() < 1000);

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

  displayVersion(NULL);
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
    cliDrawString("No SD card found. Check the adapter's wiring or try restarting.");
  }

  drawPrompt();
}

void loop() {
  myusb.Task();
  // Everything else should be a TeensyThread... eventually
  // clear screen
  // backgroundLayer.fillScreen(defaultBackgroundColor);

  if (runRGB){
    textLayer.setIndexedColor(1, rainbowColors[rgbIdx++]);
    if (rgbIdx >= RAINBOW_STEPS) rgbIdx = 0; // 0-1529
  }
  
  if (isCommandAvailable()){
      parseCommand(commandBuffer);
      Serial.println("Flush buffer");
      flushString(commandBuffer);
      commandBufferIdx = 0;
  }

  // if (inCLI == true)
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

// TODO: still a hacky solution in terms of cursor movement
void cursorBackspace() {

  commandBuffer[--commandBufferIdx] = '\0';

  MainScreen->termCursorX -= CHAR_WIDTH;
  // TODO: need much better logic for going back a line
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

  // erase pixels of erased character
  for (int x = 0; x < CHAR_WIDTH; x++){
    for (int y = 0; y < CHAR_HEIGHT; y++){
      textLayer.drawPixel(MainScreen->termCursorX + x, MainScreen->termCursorY + y, 0);
    }
  }
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
    if (tokens[1] && !strcmp("/?", tokens[1])){
      cliDrawString(command->helpInfo);
      return;
    }
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


  drawPrompt();
  Serial.println("Parse end");
}

// For assigning commands to keyboard shortcuts
void fakeCommand(const char *cmd){
  strncpy(commandBuffer, cmd, COMMAND_MAX_LENGTH-1);  // -1 to preserve null terminator?
  raiseCommandFlag();
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

// Enumerate and possibly display the files of a specified type (.XXX, all caps) in a folder
int enumerateFiles(const char *directoryName, bool displayFilenames, const char *fileType) {
  numberOfFiles = 0;
  File directory = SD.open(directoryName);
  if (!directory) {
    return -1;
  }
  while (file = directory.openNextFile()) {
    if (isFileType(file.name(), fileType)) {
      if (displayFilenames) {
        char toDisplay[64];
        snprintf(toDisplay, 63, "%d: %s", numberOfFiles + 1, file.name());
        cliDrawString(toDisplay);
      }
      numberOfFiles++;
    } else if (displayFilenames) {
      // cliDrawString("Other type: ", false);
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
  int num_files = enumerateFiles(GIF_DIRECTORY, true, ".GIF");
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
    // inCLI = false;
    // NOTE: This clears textLayer.
    // There doesn't seem to be a way to temporarily hide it.
    textLayer.fillScreen(0);
    textLayer.swapBuffers();
    // threads.addThread(gifPlayerLoop, idx);
    gifPlayerLoop(--idx);
    // on player exit...
    gfxLayer.fillScreen(bcolor2);
    gfxLayer.swapBuffers();
    clearScreen(NULL);
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
    case Enter: raiseCommandFlag(); break;
    case Backspace: cursorBackspace(); break;
    case Tab: break;
    case Esc: raiseExitFlag(); break;
    case CtrlL: fakeCommand("cls"); break;
    case CtrlAltDelete: fakeCommand("reset"); break;
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
  gfxLayer.fillScreen(bcolor2);
}

void updateScreenCallback(void) {
  gfxLayer.swapBuffers();
  // gfxLayer.swapBuffers();
  // // return;
}
void updateScreenTransparentCallback(void) {
  gfxLayer.swapBuffers();
  gfxLayer.fillScreen(bcolor2);
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
  
  if (openFilenameByIndex(GIF_DIRECTORY, index, ".GIF") >= 0) {
    if (decoder.startDecoding() < 0) {
      Serial.println("Decoder broke for some reason");
      return;
    }
    displayStartTime_millis = now;
  }

  while (!checkExitSignal()) {  // TODO: Exit after finished playing or loop
    // Decoder uses callbacks to draw the GIF
    if (decoder.decodeFrame() < 0) {
      // There's an error with this GIF, go to the next one
      Serial.println("GIF Problem");
      return;
    }
  }
}

// Teensy register-level hack to restart the program (like a power cycle or Arduino reset button)
int cpuRestart(void * args) { ( *((uint32_t *)0xE000ED0C) = 0x5FA0004); return 0; }

int displayVersion(void * args){
  cliDrawString("matrixOS [Version 1.0.0.0]");
  cliDrawString("(c) 2024 Austin S., CJ B., Jacob D.");
  return 0;
}

int clearScreen(void * args){
    textLayer.fillScreen(0);
    
    textLayer.swapBuffers();
    setCursor(0, 0);

    return 0;
}

int echoText(void * args){
  char ** arguments = (char**) args;
  while (*(++arguments)){
    cliDrawString(*arguments, false);
    cliDrawString(" ", false);
  }
  cursorNewline();
  return 0;
}


int setColor(void * args){
  char ** arguments = (char**) args;
  if (!arguments[1]){
    cliDrawString("No Color Provided");
    return 1;
  }
  rgb24 fcolor;
  rgb24 bcolor;
  char background = arguments[1][0];
  char foreground;
  bool both = (bool) arguments[1][1];
  if (!both)
    foreground = background;
  else
    foreground = arguments[1][1];



  fcolor = getColor(foreground);
  bool invalid = invalidColor;
  if (both){
    bcolor = getColor(background);
    invalid = invalid || invalidColor;
  }
  else{
    if (fcolor.red == bcolor2.red && fcolor.green == bcolor2.green && fcolor.blue == bcolor2.blue){
      invalid = true;
    }
  }


  if (!invalid && (fcolor.red != bcolor.red || fcolor.green != bcolor.green || fcolor.blue != bcolor.blue)){
    fcolor2 = fcolor;
    textLayer.setIndexedColor(1, fcolor);
    if (both){
      bcolor2 = bcolor;
      gfxLayer.fillScreen(bcolor);
      gfxLayer.swapBuffers();
    }
    return 0;
  }
  else{
    cliDrawString("Invalid Color");
  }
    return 0;
}

int bright(void * args){
  char ** arguments = (char**) args;
  if (!arguments[1]){
    cliDrawString("No brightness specified, resetting to 255");
    matrix.setBrightness(255);
    MainScreen->matrixBrightness = 255;
    return 0;
  }

  int arg1 = atoi(arguments[1]);

  if (arg1 < 10) {
    cliDrawString("Too low, setting to 10");
    arg1 = 10;
  }
  else if (arg1 > 255) {
    cliDrawString("Too high, setting to 255");
    arg1 = 255;
  }

  matrix.setBrightness(arg1);
  MainScreen->matrixBrightness = arg1;

  return 0;
}

int cliRGB(void * args) {
  runRGB = !runRGB;
  if (runRGB)
    cliDrawString("TASTE THE RAINBOW");
  else {
    cliDrawString("No longer tasting the rainbow");
    textLayer.setIndexedColor(1, fcolor2);
  }
  return 0;
}