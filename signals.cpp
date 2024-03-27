#include "signals.h"

static bool commandAvailable = false;

static bool exitFlag = false;

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