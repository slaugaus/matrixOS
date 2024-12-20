#ifndef SIGNALS_H_
#define SIGNALS_H_

void raiseCommandFlag(void);
bool isCommandAvailable(void);
void raiseExitFlag(void);
bool checkExitSignal(void);

void raiseLeftFlag(void);
bool isLeftPressed(void);

void raiseRightFlag(void);

bool isRightPressed(void);


#endif