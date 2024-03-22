#include "CommandTable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>




static Command ** cmdTable = NULL;
static unsigned long tableSize = 0;
static bool isTableInitialized = false;



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

Command * genCommand(char * title, char * helpInfo, fpointer function){
	Command * command = (Command*) malloc(sizeof(Command));
	if (!command) return NULL;
	command->title = title;
	command->helpInfo = helpInfo;
	command->function = function;
	command->next = NULL;
	return command;
}

bool appendCommand(char * title, char * helpInfo, fpointer function){
	if (!isTableInitialized){
		return false;
	}
	Command * command = genCommand(title, helpInfo, function);
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
	unsigned long index = getHash(title);
	if (!cmdTable[index])
		return NULL;

	Command * cursor = cmdTable[index];

	do{
		if(!strcmp(cursor->title, title))
			return cursor;		// Strings match, return function
		cursor = cursor->next;
	}while (cursor != NULL);

	return NULL;
}
