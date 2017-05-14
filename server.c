#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "volkov.h"


#define MAXCL 64
#define MAXRM 64

enum roomerstate {
	SET_NAME = 0,
	CH_ROOM,
	NEW_ROOM,
	READY,
	LEAVE
};

enum broadtype {
	ENTR = 0,
	LVNG,
	TEXT,
	SERV
};

struct soc_inf {
	int sockfd;
	int len;
	struct sockaddr_in address;
};

struct room {
	struct roomer *members[10];
	int counter;
	char name[STRLEN];
	int id;
};

struct roomer {
	struct soc_inf sockinf;
	char name[STRLEN];
	char ipport[STRLEN];
	struct room *curroom;
	int state;
	int id;
	pthread_t readthread;
};


struct roomer **clients;
struct room **rooms;


void broadcast(struct roomer *proomer, int type, char *msg)
{
	int i = 0;
	char buff[STRLEN] = {0};
	struct room *proom;

	if (proomer != NULL)
		proom = proomer->curroom;
	switch (type) {
	case ENTR:
		strcpy(buff, "[ROOM] New roomer here: ");
		strcat(buff, proomer->name);
		strcat(buff, "\n");
		for (i = 0; i < proom->counter; i++) {
			if (proom->members[i] != proomer)
				write(proom->members[i]->sockinf.sockfd,
					buff, strlen(buff));
		}
		break;
	case LVNG:
		strcpy(buff, "[ROOM] Say goodbye to: ");
		strcat(buff, proomer->name);
		strcat(buff, "\n");
		for (i = 0; i < proom->counter; i++) {
			if (proom->members[i] != proomer)
				write(proom->members[i]->sockinf.sockfd,
					buff, strlen(buff));
		}
		break;
	case TEXT:
		strcpy(buff, proomer->name);
		strcat(buff, " > ");
		strcat(buff, msg);
		for (i = 0; i < proom->counter; i++) {
			if (proom->members[i] != proomer)
				write(proom->members[i]->sockinf.sockfd,
					buff, strlen(buff));
		}
		break;
	case SERV:
		strcpy(buff, "[SERVER] ");
		strcat(buff, msg);
		for (i = 1; i < MAXCL; i++)
			if (clients[i] != NULL)
				write(clients[i]->sockinf.sockfd,
						buff, strlen(buff));
		break;
	default:
		break;
	}
}

void enter_room(struct roomer *proomer)
{
	char buff[STRLEN] = {0};
	char tempbuff[STRLEN] = {0};
	int i;

	proomer->curroom->counter++;
	strcpy(buff, "You entered room \"");
	strcat(buff, proomer->curroom->name);
	strcat(buff, "\" [");
	strcat(buff, longtostring(proomer->curroom->counter, tempbuff));
	strcat(buff, "/10]\n");
	write(proomer->sockinf.sockfd, buff, strlen(buff));
	strcpy(buff, "Roomers: ");
	for (i = 0; i < proomer->curroom->counter; i++) {
		strcat(buff, proomer->curroom->members[i]->name);
		strcat(buff, ", ");
	}
	buff[strlen(buff)-2] = 0;
	strcat(buff, "\nType /leave for leave.\n");
	strcat(buff, "Be polite!\n");
	write(proomer->sockinf.sockfd, buff, strlen(buff));

	strcpy(buff, proomer->name);
	strcat(buff, ": moved to room: ");
	strcat(buff, proomer->curroom->name);
	println(buff);

	broadcast(proomer, ENTR, NULL);

	proomer->state = READY;
}


void select_room(struct roomer *proomer)
{
	char buff[STRLEN] = {0};
	char tempbuff[STRLEN] = {0};
	int i;

	strcpy(buff, "Choose room or create new one\n");
	write(proomer->sockinf.sockfd, buff, strlen(buff));
	for (i = 1; i < MAXRM; i++) {
		if (rooms[i] != NULL) {
			strcpy(buff, longtostring(rooms[i]->id, tempbuff));
			strcat(buff, ") ");
			strcat(buff, rooms[i]->name);
			strcat(buff, "\n");
			write(proomer->sockinf.sockfd, buff, strlen(buff));
		}
	}
	strcpy(buff, "n) *create new*\n");
	write(proomer->sockinf.sockfd, buff, strlen(buff));
}


void leave_room(struct roomer *proomer)
{
	char buff[STRLEN] = {0};
	int i;

	broadcast(proomer, LVNG, NULL);

	proomer->curroom->counter--;
	i = 0;
	while (proomer->curroom->members[i] != proomer) {
		i++;
		if (i >= 9)
			operror(LEAVING);
	}
	for (i; i < 9; i++)
		proomer->curroom->members[i] = proomer->curroom->members[i+1];

	strcpy(buff, proomer->name);
	strcat(buff, ": left room: ");
	strcat(buff, proomer->curroom->name);
	println(buff);

	proomer->curroom = NULL;
	proomer->state = CH_ROOM;
}


void *reader(void *arg)
{
	struct roomer *proomer;
	char buff[STRLEN] = {0};
	int roomsel = 0;

	proomer = (struct roomer *)arg;
	while (1) {
		int nread;
		int i = 0;
		int j = 0;

		memset(buff, 0, STRLEN);
		nread = read(proomer->sockinf.sockfd, buff, STRLEN);
		j = checkbuff(buff, nread);
		if (j) {
			if (j == 1)
				strcpy(buff, "Type normal symbols, please.\n");
			if (j == 2)
				strcpy(buff, "Empty input is not allowed.\n");
			write(proomer->sockinf.sockfd, buff, strlen(buff));
			continue;
		}
		if (nread == 0) {
			if (proomer->curroom != NULL)
				leave_room(proomer);

			print("Connection with ");
			print(proomer->ipport);
			print(" [");
			print(proomer->name);
			print("] ");
			println("has been lost.");
			close(proomer->sockinf.sockfd);
			free(clients[proomer->id]);
			pthread_exit("0");
		}


		if (proomer->state == READY &&
			strncmp(buff, "/leave", 6) == 0)
			proomer->state = LEAVE;
		switch (proomer->state) {
		case SET_NAME:
			strcpy(proomer->name, buff);
			proomer->name[strlen(proomer->name)-1] = 0;
			print(proomer->ipport);
			print(" is set name to \"");
			print(proomer->name);
			println("\"");

			select_room(proomer);

			proomer->state = CH_ROOM;
			break;
		case CH_ROOM:
			if (buff[0] == 'n') {
				strcpy(buff, "Write room name: ");
				write(proomer->sockinf.sockfd,
						buff, strlen(buff));
				proomer->state = NEW_ROOM;
				break;
			}
			roomsel = strtol(buff, NULL, 10);
			if (rooms[roomsel] != NULL) {
				j = rooms[roomsel]->counter;
				if (j < 10) {
					rooms[roomsel]->members[j] = proomer;
					proomer->curroom = rooms[roomsel];

					enter_room(proomer);
				} else {
					strcpy(buff, "This room is full.\n");
					write(proomer->sockinf.sockfd,
						buff, strlen(buff));
					break;
				}
			}
			if (proomer->state != READY) {
				strcpy(buff, "Please, type correctly.\n");
				write(proomer->sockinf.sockfd,
					buff, strlen(buff));
			}
			break;
		case NEW_ROOM:
			i = 1;
			while (rooms[i] != NULL) {
				i++;
				if (i >= MAXRM)
					operror(NW_ROOM);
			}
			rooms[i] = (struct room *)calloc(1,
					sizeof(struct room));
			rooms[i]->id = i;
			buff[nread-1] = 0;
			strcpy(rooms[i]->name, buff);

			rooms[i]->members[0] = proomer;
			proomer->curroom = rooms[i];

			strcpy(buff, "New room created: ");
			strcat(buff, rooms[i]->name);
			println(buff);

			enter_room(proomer);

			break;
		case READY:
			print(proomer->ipport);
			print(" [");
			print(proomer->name);
			print("] > ");
			write(1, buff, nread);

			broadcast(proomer, TEXT, buff);

			break;
		case LEAVE:
			leave_room(proomer);

			select_room(proomer);

			break;
		default:
			write(1, buff, nread);
			break;
		}
	}
}


void *listener(void *arg)
{
	int len;
	struct sockaddr_in address;
	struct soc_inf *server;
	struct roomer *proomer;
	char buff[STRLEN] = {0};

	println("listener created");

	server = (struct soc_inf *)arg;
	len = sizeof(address);
	while (1) {
		int sockfd;
		int i;

		sockfd = accept(server->sockfd,
					(struct sockaddr *)&address,
					&len);
		if (sockfd == -1)
			operror(ACCEPT);

		i = 0;
		while (clients[i] != NULL) {
			i++;
			if (i >= MAXCL) {
				println("too much roomers! Reject\n");
				break;
			}
		}
		if (i >= MAXCL)
			continue;
		clients[i] = (struct roomer *)calloc(1, sizeof(struct roomer));
		proomer = clients[i];
		proomer->sockinf.sockfd = sockfd;
		address.sin_addr.s_addr = htonl(address.sin_addr.s_addr);
		address.sin_port = htons(address.sin_port);
		proomer->sockinf.address = address;
		proomer->sockinf.len = len;
		proomer->id = i;
		print("New roomer: ");
		strcpy(proomer->ipport,
				IPtostring(proomer->sockinf.address, buff));
		println(proomer->ipport);

		strcpy(buff, "\nWelcome to Chatroom!\n");
		write(sockfd, buff, strlen(buff));
		strcpy(buff, "Please, type your name: ");
		write(sockfd, buff, strlen(buff));

		pthread_create(&proomer->readthread, NULL,
						reader, (void *)(proomer));
	}
}

int main(int argc, char *argv[])
{
	struct soc_inf serv;
	pthread_t listener_thread;
	int res;
	char buff[STRLEN];

	clients = (struct roomer **)calloc(MAXCL, sizeof(struct roomer *));
	rooms = (struct room **)calloc(MAXRM, sizeof(struct room *));

	signal(SIGINT, SIG_IGN);

	serv.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serv.sockfd == -1)
		operror(SOCKET);

	serv.address.sin_family = AF_INET;
	serv.address.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.address.sin_port = htons(40304);
	serv.len = sizeof(serv.address);

	res = bind(serv.sockfd, (struct sockaddr *)&serv.address, serv.len);
	if (res != 0)
		operror(BIND);

	listen(serv.sockfd, 5);

	pthread_create(&listener_thread, NULL, listener, (void *)(&serv));

	while (1) {
		int nread;

		memset(buff, 0, STRLEN);
		nread = read(0, buff, STRLEN);
		if (strncmp(buff, "/exit", 5) == 0 || nread == 0)
			break;
		broadcast(NULL, SERV, buff);
	}

	return 0;
}
