/*
 * Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels
 *
 * This file contains code to enumerate and select animated GIF files by name
 *
 * Written by: Craig A. Lindley
 */

#ifndef FILENAME_FUNCTIONS_H
#define FILENAME_FUNCTIONS_H

int enumerateGIFFiles(const char *directoryName, bool displayFilenames);
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer);
int openGifFilenameByIndex(const char *directoryName, int index);
int initFileSystem(int chipSelectPin);

bool isAnimationFile(const char filename[]);

bool fileSeekCallback(unsigned long position);
unsigned long filePositionCallback(void);
int fileReadCallback(void);
int fileReadBlockCallback(void *buffer, int numberOfBytes);
int fileSizeCallback(void);

extern int numberOfFiles;
#include <SD.h>
extern File file;

#endif
