#ifndef COMMAND_TABLE_H_

#define COMMAND_TABLE_H_

#include <stdbool.h>

typedef int (*fpointer) (void *);

typedef struct Command{
	char * title;
	char * helpInfo;
	fpointer function;
	struct Command * next;
}Command;


bool initCMDTable(unsigned long size);

unsigned long long getHash(char * str);

Command * genCommand(const char * title, const char * helpInfo, fpointer function);

bool appendCommand(const char * title, const char * helpInfo, fpointer function);

void replaceNextSpace(char * str);

Command* getCommand(char * title);

bool GetTableStatus(void);

#endif
