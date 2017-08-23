
/* A simple client program for server.c

To compile: gcc client.c -o client 

To run: start the server, then the client */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h> 

pthread_t tid[100];
int sockfd,tInUse[100];
pthread_mutex_t lock,lockTime;

void *threadFunc(void *);

int main(int argc, char**argv)
{
	int portno, n,i;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[256];
	
	memset(tInUse, 0, sizeof(tInUse));

	if (argc < 3) 
	{
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);

	if (pthread_mutex_init(&lock, NULL) || pthread_mutex_init(&lockTime, NULL) != 0)
	{
		printf("mutex init failed\r\n\r\n");
		exit(1);
	}
	
	
	/* Translate host name into peer's IP address ;
	* This is name translation service by the operating system 
	*/
	server = gethostbyname(argv[1]);
	
	if (server == NULL) 
	{
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	
	/* Building data structures for socket */

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, 
	(char *)&serv_addr.sin_addr.s_addr,
	server->h_length);

	serv_addr.sin_port = htons(portno);

	/* Create TCP socket -- active open 
	* Preliminary steps: Setup: creation of active open socket
	*/
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(0);
	}
	
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
	{
		perror("ERROR connecting");
		exit(0);
	}


	/* Do processing
	*/
	while(1)
	{


		printf("Please enter the message: ");

		bzero(buffer,256);

		strcpy(buffer,"WORK 1d29ffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f a900000003b4ab00 04\r\n");
		
		//fgets(buffer,255,stdin);
	
		n = write(sockfd,buffer,strlen(buffer));

		if (n < 0) 
		{
			perror("ERROR writing to socket");
			exit(0);
		}
		printf("%s\n",buffer);
		
		usleep(1000000);
		bzero(buffer,256);

		n = recv(sockfd,buffer,255,MSG_DONTWAIT);
		//n = read(sockfd,buffer,255);
		
/* 		if (n < 0)
		{
			perror("ERROR reading from socket");
			exit(0);
		} */

		printf("%s\n",buffer);
		//sleep(1);
		//usleep(300000);
	}


	return 0;
}

