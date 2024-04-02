#include <Arduino.h>
// enables serial prints from the extern "C" block
void Serialprintln(const char *msg) {Serial.println(msg);}
void Serialprint(const char *msg) {Serial.print(msg);}

// #ifdef __cplusplus
 extern "C" {
// #endif

#include "CommandTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static Command ** cmdTable = NULL;
static unsigned long tableSize = 0;
static bool isTableInitialized = false;

unsigned long getTableSize(void){
  return tableSize;
}

Command ** getTable(void){
  return cmdTable;
}

// Returns status of successfully sizing command table, argument is number of index locations for table
bool initCMDTable(unsigned long size){
	if (size){
		isTableInitialized = true;
		cmdTable = (Command**) realloc((void*)cmdTable, sizeof(Command*) * size);
		if (!cmdTable){
			isTableInitialized = false;
			tableSize = 0;
			return false;
		}
		memset(cmdTable, 0, sizeof(Command*) * size);  // Initializes table to NULL
		tableSize = size;
		return true;
	}
	else{
		isTableInitialized = false;
		cmdTable = (Command**) realloc((void*)cmdTable, 0);
		tableSize = 0;
		return true;
	}
}

bool GetTableStatus(void){
	return isTableInitialized;
}

unsigned long long getHash(char * str){
	int c;
	unsigned long long hash = 0;
	while (c = *(str++)){
		hash = c + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

Command * genCommand(const char * title, const char * helpInfo, const char * longHelp, fpointer function){
	Command * command = (Command*) malloc(sizeof(Command));
	if (!command) return NULL;
	command->title = title;
	command->helpInfo = helpInfo;
  command->longHelp = longHelp;
	command->function = function;
	command->next = NULL;
	return command;
}

bool appendCommand(const char * title, const char * helpInfo, const char * longHelp, fpointer function){
	if (!isTableInitialized){
		return false;
	}
	Command * command = genCommand(title, helpInfo, longHelp, function);
	if (!command) return false;

	unsigned long index = getHash(command->title) % tableSize;

	if (cmdTable[index]){
		Command * cursor = cmdTable[index];
		while(cursor->next != NULL){
			if (!strcmp(cursor->title, command->title)){
				free(command);
				return false;
			}
			cursor = cursor->next;
		}
		if (!strcmp(cursor->title, command->title)){
			free(command);
			return false;
		}

		cursor->next = command;
		return true;

	}
	else{
		cmdTable[index] = command;
		return true;
	}
	return false;
}



Command * getCommand(char * title){
  Serialprint("getting: ");
  Serialprintln(title);
	unsigned long long index = getHash(title) % tableSize;
	if (!cmdTable[index]){
    Serialprintln("getCommand returning null 1");
		return NULL;
  }

	Command * cursor = cmdTable[index];
  Serialprintln("getCommand left getHash");

	do{
		if(!strcmp(cursor->title, title)){
			return cursor;		// Strings match, return function
    }
		cursor = cursor->next;
	}while (cursor != NULL);
  Serialprintln("getCommand returning null 2");
	return NULL;
}

// #ifdef __cplusplus
}
// #endif
