/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

#include "FilenameFunctions.h"

// #include <SD.h>
int numberOfFiles;
File file;

// Chip select for SD card
#define SD_CS BUILTIN_SDCARD

bool fileSeekCallback(unsigned long position) {
  return file.seek(position);
}

unsigned long filePositionCallback(void) {
  return file.position();
}

int fileReadCallback(void) {
  return file.read();
}

int fileReadBlockCallback(void *buffer, int numberOfBytes) {
  return file.read((uint8_t *)buffer, numberOfBytes);
}

int fileSizeCallback(void) {
  return file.size();
}

int initFileSystem(int chipSelectPin) {
  // initialize the SD card at full speed
  if (chipSelectPin >= 0) {
    pinMode(chipSelectPin, OUTPUT);
  }
  if (!SD.begin(chipSelectPin))
    return -1;
  return 0;
}

bool isFileType(const char filename[], const char extension[]) {
  String filenameString(filename);

  if ((filenameString[0] == '_') || (filenameString[0] == '~') || (filenameString[0] == '.')) {
    return false;
  }

  filenameString.toUpperCase();
  if (filenameString.endsWith(extension) != 1)
    return false;

  return true;
}

// Get the full path/filename of the GIF file with specified index
void getFilenameByIndex(const char *directoryName, int index, char *pnBuffer, const char *extension) {

  // Serial.printf("(get)  Dir: %s\tExt: %s\tIdx: %d\n", directoryName, extension, index);
  // Make sure index is in range
  if ((index < 0) || (index >= numberOfFiles))
    return;

  File directory = SD.open(directoryName);
  if (!directory)
    return;

  // Serial.println("Dir opened");

  while ((index >= 0)) {
    // Serial.printf("Opening %d\n", index);
    file = directory.openNextFile();
    if (!file) break;

    if (isFileType(file.name(), extension)) {
      // Serial.printf("%s Is a %s\n", file.name(), extension);
      index--;

      // Copy the directory name into the pathname buffer
      strcpy(pnBuffer, directoryName);

      int len = strlen(pnBuffer);
      if (len == 0 || pnBuffer[len - 1] != '/') strcat(pnBuffer, "/");

      // Append the filename to the pathname
      strcat(pnBuffer, file.name());
    }

    file.close();
  }

  file.close();
  directory.close();
}

int openFilenameByIndex(const char *directoryName, int index, const char *extension) {
  char pathname[255];

  // Serial.printf("(open) Dir: %s\tExt: %s\tIdx: %d\n", directoryName, extension, index);

  getFilenameByIndex(directoryName, index, pathname, extension);

  // Serial.print("Pathname: ");
  // Serial.println(pathname);

  if (file)
    file.close();

  // Attempt to open the file for reading
  file = SD.open(pathname);
  if (!file) {
    // Serial.println("Error opening file");
    return -1;
  }

  return 0;
}

// Callbacks for PNGDec and JPEGDec (Larry Bank's libs)
void * bankOpen(const char *filename, int32_t *size){
  // Serial.printf("Attempting to open %s\n", filename);
  file = SD.open(filename);
  *size = file.size();
  return &file;
}

void bankClose(void *handle){
  if (file) file.close();
}

int32_t bankRead(void *handle, uint8_t *buf, int32_t length){
  if (!file) return 0;
  return file.read(buf, length);
}

int32_t bankSeek(void *handle, int32_t pos){
  if (!file) return 0;
  return file.seek(pos);
}


// Return a random animated gif path/filename from the specified directory
// void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer) {

//   int index = random(numberOfFiles);
//   getGIFFilenameByIndex(directoryName, index, pnBuffer);
// }
