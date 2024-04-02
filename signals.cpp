#include "signals.h"

static bool commandAvailable = false;

static bool exitFlag = false;

static bool leftFlag = false;
static bool rightFlag = false;

void raiseLeftFlag(void){
  leftFlag = true;
}

bool isLeftPressed(void){
  if (leftFlag){
    leftFlag = false;
    return true;
  }
  return false;
}

void raiseRightFlag(void){
  rightFlag = true;
}

bool isRightPressed(void){
  if (rightFlag){
    rightFlag = false;
    return true;
  }
  return false;
}

void raiseCommandFlag(void){
  commandAvailable = true;
}

bool isCommandAvailable(void){
  if (commandAvailable){
    commandAvailable = false;
    return true;
  }
  return false;
}


void raiseExitFlag(void){
  exitFlag = true;
}

bool checkExitSignal(void){
  if (exitFlag){
    exitFlag = false;
    return true;
  }
  return false;
}