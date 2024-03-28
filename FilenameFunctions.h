/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

#ifndef FILENAME_FUNCTIONS_H
#define FILENAME_FUNCTIONS_H

void getFilenameByIndex(const char *directoryName, int index, char *pnBuffer, const char *extension);
int openFilenameByIndex(const char *directoryName, int index, const char *extension);
int initFileSystem(int chipSelectPin);

bool isFileType(const char filename[], const char extension[]);

bool fileSeekCallback(unsigned long position);
unsigned long filePositionCallback(void);
int fileReadCallback(void);
int fileReadBlockCallback(void *buffer, int numberOfBytes);
int fileSizeCallback(void);

// extern int numberOfFiles;
#include <SD.h>
extern File file;

#endif
