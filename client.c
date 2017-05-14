#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#include "volkov.h"

void *reader(void *arg)
{
	int *sockfd;
	char buff[STRLEN];

	sockfd = (int *)arg;
	while (1) {
		int nread;

		nread = read(*sockfd, buff, STRLEN);
		if (nread != 0)
			write(1, buff, nread);
	}
}

int main(int argc, char *argv[])
{
	int sockfd;
	int len;
	struct sockaddr_in address;
	pthread_t reader_thread;
	int nread = 0;
	char buff[STRLEN] = {0};
	int status;
	int ipcfg;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	pthread_create(&reader_thread, NULL, reader, (void *)(&sockfd));

	println("Type \"/exit\" anywhere to exit.");

	ipcfg = open("ip.cfg", O_RDONLY);
	if (ipcfg == -1) {
		println("Write server IP and port, please.");
		println("Ask system administrator if some problem occured.");
		print("IP: ");
		nread = read(0, buff, 15);
		if (nread < 7)
			return 1;
		ipcfg = creat("ip.cfg", S_IWUSR | S_IRUSR);
		if (ipcfg != -1) {
			write(ipcfg, buff, 15);
			address.sin_addr.s_addr = inet_addr(buff);
			print("Port: ");
			memset(buff, 0, STRLEN);
			read(0, buff, 6);
			nread = strtol(buff, NULL, 10);
			address.sin_port = htons(nread);
			write(ipcfg, buff, 6);
			close(ipcfg);
		}
	} else {
		nread = read(ipcfg, buff, 15);
		if (nread < 7) {
			println("wrong ip!");
			exit(1);
		}
		address.sin_addr.s_addr = inet_addr(buff);
		memset(buff, 0, STRLEN);
		read(ipcfg, buff, 6);
		nread = strtol(buff, NULL, 10);
		address.sin_port = htons(nread);
		close(ipcfg);
	}

	address.sin_family = AF_INET;
	len = sizeof(address);

	status = connect(sockfd, (struct sockaddr *)&address, len);

	if (status == -1)
		println("Wrong IP or server is down.");

	while (1) {
		nread = read(0, buff, STRLEN);
		if (strncmp(buff, "/exit", 5) == 0) {
			close(sockfd);
			break;
		}
		write(sockfd, buff, nread);
	}

	return 0;
}
