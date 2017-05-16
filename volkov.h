#ifndef VOLKOV_H
#define VOLKOV_H

#define STRLEN 128

#include <netinet/in.h>

enum errorCode {
	SOCKET = 1,
	BIND,
	ACCEPT,
	LEAVING,
	NW_ROOM,
	MUTEX
};


void operror(char code);

void print(char *str);

void println(char *str);

char *longtostring(long x, char* buff);

void printlong(long x);

char *IPtostring(struct sockaddr_in address, char *string);

char checkbuff(char *buff, int length);

#endif
