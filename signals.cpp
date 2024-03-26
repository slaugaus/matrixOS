#include "signals.h"

static bool commandAvailable = false;

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