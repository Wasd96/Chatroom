#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "volkov.h"

void operror(char code)
{
	char str[STRLEN] = {0};

	switch (code) {
	case 1:
		strcpy(str, "Error: create socket\n");
		break;
	case 2:
		strcpy(str, "Error: bind socket\n");
		break;
	case 3:
		strcpy(str, "Error: accept\n");
		break;
	case 4:
		strcpy(str, "Error: cant find in members\n");
		break;
	case 5:
		strcpy(str, "Error: cant find created room\n");
		break;
	case 6:
		strcpy(str, "Error: init mutex\n");
		break;
	default:
		strcpy(str, "Some error occured\n");
	}
	write(2, str, strlen(str));
	exit(code);
}

void print(char *str)
{
	write(1, str, strlen(str));
}

void println(char *str)
{
	print(str);
	write(1, "\n", 1);
}

char *longtostring(long x, char *buff)
{
	int i = STRLEN-1;
	int j = 0;
	char neg = 0;

	memset(buff, 0, STRLEN);
	if (x < 0)
		neg = 1;
	while (x > 0) {
		char c;

		c = '0'+x%10;
		buff[i] = c;
		x = x/10;
		i--;
	}
	if (neg) {
		buff[i] = '-';
		i--;
	}
	for (j = 0; i < STRLEN-1; j++, i++)
		buff[j] = buff[i+1];
	return buff;
}

/*
*void printlong(long x)
*{
*	char buff[STRLEN];
*
*	print(longtostring(x, buff));
*}
*/


char *IPtostring(struct sockaddr_in address, char *string)
{
	char buff[STRLEN] = { 0 };
	char pbuff[STRLEN] = { 0 };
	int i;

	memset(string, 0, STRLEN);
	for (i = 0; i < 4; i++) {
		unsigned char ipbyte;
		unsigned int bytetoint;

		ipbyte = (address.sin_addr.s_addr >> (3-i)*8) & 0xFF;
		bytetoint = ipbyte;
		strcpy(pbuff, longtostring(bytetoint, buff));
		strcat(string, pbuff);
		if (i != 3)
			strcat(string, ".");
	}
	strcat(string, ":");
	strcat(string, longtostring(address.sin_port, buff));

	return string;
}


char checkbuff(char *buff, int length)
{
	int i = 0;

	for (i; i < length-1; i++)
		if (buff[i] < 32 || buff[i] > 126)
			return 1;
	if (length == 1)
		return 2;
	return 0;
}

