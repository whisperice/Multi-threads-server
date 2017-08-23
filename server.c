// COMP30023 Computer Systems Project 2
// Name: Yitao Deng
// User ID: yitaod
// Student ID: 711645


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
#include "uint256.h"
#include "sha256.h"
#include "server.h"

//global variables
time_t timer;
struct tm *timeInfo;
char timeStr[30];
FILE *fp;
int tInUse[maxCli], tCount=0, worktInUse[maxWorker]; 
pthread_t tid[maxCli], worktid[maxWorker];
pthread_mutex_t lock,lockTime;
workQP workQue;
wtPar *workingTh=NULL;


int main(int argc, char **argv)
{
	int sockfd, connfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int i;		
	tPar *thPar;
	char timeStrbuf[maxbuf];
	char cliIP[INET_ADDRSTRLEN];
	
	memset(tInUse, 0, sizeof(tInUse));
	memset(worktInUse, 0, sizeof(worktInUse));
	clilen = sizeof(cli_addr);
	
	//init workQue, has Head element
	workQue=(workQP)malloc(sizeof(workQE));
	workQue->next=NULL;
	
	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\r\n\r\n");
		exit(1);
	}

	if ( (fp=fopen("log.txt", "w") ) == NULL)
	{
		printf("Open file failed\r\n\r\n");
		exit(1);
	}
	setlinebuf(fp);
	
	if (pthread_mutex_init(&lock, NULL) || pthread_mutex_init(&lockTime, NULL) != 0)
	{
		printf("mutex init failed\r\n\r\n");
		exit(1);
	}
	
	//Create TCP socket
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	
	pthread_mutex_lock(&lockTime);
	getTime();
	sprintf(timeStrbuf,"[%s]Creating TCP socket at port %d...\r\n\r\n", timeStr, portno);
	pthread_mutex_unlock(&lockTime);
	
	fprintf(fp, "%s",timeStrbuf);
	printf("%s", timeStrbuf);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) 
	{
		perror("ERROR opening socket");
		exit(1);
	}
	
	
	//Create address we're going to listen on (given port number)
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	//Bind address to the socket
	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0) 
	{
		perror("ERROR on binding");
		exit(1);
	}
	
	//Listen on socket - means we're ready to accept connections - 
	pthread_mutex_lock(&lockTime);
	getTime();
	sprintf(timeStrbuf,"[%s]Waiting for a client at port %d...\r\n\r\n", timeStr, portno);
	pthread_mutex_unlock(&lockTime);
	
	fprintf(fp, "%s",timeStrbuf);
	printf("%s", timeStrbuf);

	listen(sockfd,maxCli);
	
	//Accept connections
waitForCli:	
	while(1)
	{
		pthread_mutex_lock(&lock);
		for(i = 0; i < maxCli;i ++)
		{
			if(tInUse[i] == 0)
			{
				tInUse[i] = 1;
				tCount++;
				pthread_mutex_unlock(&lock);
				break;
			}
			else if(i == maxCli-1)
			{
				if(tCount !=maxCli)
				{
					pthread_mutex_unlock(&lock);
					goto waitForCli;
				}
				else	//reject new client
				{
					pthread_mutex_unlock(&lock);
					
					connfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen);
					
					if(connfd >0)
					{						
/* 						inet_ntop(AF_INET, &(cli_addr.sin_addr), cliIP, sizeof(cliIP) );
						pthread_mutex_lock(&lockTime);
						getTime();
						sprintf(timeStrbuf,"[%s]ERRO :Reach max client number. Reject the client at %s.\r\n\r\n",timeStr, cliIP);
						pthread_mutex_unlock(&lockTime);
						
						fprintf(fp, "%s",timeStrbuf);
						printf("%s", timeStrbuf);
						write(connfd,timeStrbuf,sizeof(timeStrbuf)-1 ); */
						close(connfd);
					}
					goto waitForCli;
				}
			}
		}
		
		connfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen);

		if (connfd < 0) 
		{
			pthread_mutex_lock(&lockTime);
			getTime();
			sprintf(timeStrbuf,"[%s]ERROR on accept a new client.\r\n\r\n",timeStr);			
			fprintf(fp, "%s",timeStrbuf);
			printf("%s", timeStrbuf);
			tInUse[i] = 0;
			tCount--;
			pthread_mutex_unlock(&lockTime);
			goto waitForCli;
		}
		
		//initialize the client thread parameter
		thPar = (tPar *)malloc(sizeof(tPar));
		thPar -> cliSockId = connfd;
		thPar -> thIndex = i;
		memcpy(&(thPar->cliAddr), &cli_addr, sizeof(struct sockaddr_in));
		
		pthread_create(&(tid[i]), NULL, threadFunc, (void *)thPar);
		
	}
	
	
	/* close socket */
	close(sockfd);
	
	return 0; 
}

//client thread function
void *threadFunc(void *threadParam) 
{
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	
	int n,nerr;
	char readbuf[maxbuf], writebuf[maxbuf];

	tPar *thPar = (tPar *)threadParam;
	
	//pthread_cleanup_push(cleanupCli, threadParam);
	
	char cliIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(thPar->cliAddr.sin_addr), cliIP, sizeof(cliIP) );
/* 	pthread_mutex_lock(&lockTime);
	getTime();
	fprintf(fp,"[%s]There is a new client at %s .\r\n\r\n", timeStr, cliIP);
	printf("[%s]There is a new client at %s .\r\n\r\n", timeStr, cliIP);
	pthread_mutex_unlock(&lockTime); */
	
	while(1)
	{
		
		//read from client
		bzero(readbuf,sizeof(readbuf));
		nerr=0;
readagain:		

		pthread_testcancel();
		n = read(thPar->cliSockId, readbuf, 100);
				
		if (n < 0) 
		{
			if (errno == EINTR)
			{
				goto readagain;
			}
			else
			{
				pthread_mutex_lock(&lockTime);
				/* getTime();
				fprintf(fp, "[%s]ERROR read from the client at %s.\r\n\r\n", timeStr, cliIP);
				printf("[%s]ERROR read from the client at %s.\r\n\r\n", timeStr, cliIP); */
				tInUse[thPar->thIndex] = 0;	
				tCount--;
				close(thPar->cliSockId);
				pthread_mutex_unlock(&lockTime);
				pthread_exit(NULL);	
				/* nerr++;
				if(nerr <= 3)
				{
					pthread_mutex_lock(&lockTime);
					getTime();
					fprintf(fp, "[%s]Retry to read from the client at %s, time %d.\r\n\r\n", timeStr, cliIP, nerr);
					printf("[%s]Retry to read from the client at %s, time %d.\r\n\r\n", timeStr, cliIP, nerr);
					pthread_mutex_unlock(&lockTime);
					sleep(1);
					goto readagain;
				}
				else
				{
					pthread_mutex_lock(&lockTime);
					getTime();
					fprintf(fp, "[%s]Retry to read from the client at %s too many times; End connection.\r\n\r\n", timeStr, cliIP);
					printf("[%s]Retry to read from the client at %s too many times; End connection.\r\n\r\n", timeStr, cliIP);
					tInUse[thPar->thIndex] = 0;	
					tCount--;
					close(thPar->cliSockId);
					pthread_mutex_unlock(&lockTime);
					pthread_exit(NULL);					
				}*/
			}		 
		}
		else if (n == 0)
		{
			pthread_mutex_lock(&lockTime);
/* 			getTime();
			fprintf(fp,"[%s]the client at %s has been closed.\r\n\r\n", timeStr, cliIP);
			printf("[%s]the client at %s has been closed.\r\n\r\n", timeStr, cliIP); */
			tInUse[thPar->thIndex] = 0;	
			tCount--;
			close(thPar->cliSockId);
			pthread_mutex_unlock(&lockTime);
			pthread_exit(NULL);
		}
		
		pthread_mutex_lock(&lockTime);
		getTime();
		fprintf(fp,"[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, cliIP, thPar->cliSockId, readbuf);
		printf("[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, cliIP, thPar->cliSockId, readbuf);
		pthread_mutex_unlock(&lockTime);

		//write to client
		bzero(writebuf,sizeof(writebuf));
		
		getResponse(readbuf,writebuf,n,thPar);
		
		nerr=0;
writeagain:				
		
		pthread_testcancel();	
		
		//if(1)
		if(strncmp(readbuf,"WORK ",5) != 0)
		{
			if( (strncmp(writebuf,"PONG\r\n",6) == 0) || (strncmp(writebuf,"OKAY\r\n",6) == 0) )
			{
				n = write(thPar->cliSockId, writebuf , 6);
			}
			else if(strncmp(writebuf,"ERRO",4) == 0) 
			{
				n = write(thPar->cliSockId, writebuf ,40);
			}
			else
			{
				n = write(thPar->cliSockId, writebuf, sizeof(writebuf)-1);
			}
			
			if (n < 0) 
			{
				if (errno == EINTR)
				{
					goto writeagain;
				}
				else
				{
					pthread_mutex_lock(&lockTime);
					tInUse[thPar->thIndex] = 0;	
					tCount--;
					close(thPar->cliSockId);
					pthread_mutex_unlock(&lockTime);
					pthread_exit(NULL);	
					/* getTime();
					fprintf(fp, "[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
					printf("[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP); */
					/* pthread_mutex_unlock(&lockTime);
					nerr++;
					if(nerr <= 3)
					{
						pthread_mutex_lock(&lockTime);
						getTime();
						fprintf(fp, "[%s]Retry to write to the client at %s, time %d.\r\n\r\n", timeStr, cliIP, nerr);
						printf("[%s]Retry to write to the client at %s, time %d.\r\n\r\n", timeStr, cliIP, nerr);
						pthread_mutex_unlock(&lockTime);
						sleep(1);
						goto writeagain;
					}
					else
					{
						pthread_mutex_lock(&lockTime);
						getTime();
						fprintf(fp, "[%s]Retry to write to the client at %s too many times; End connection.\r\n\r\n", timeStr, cliIP);
						printf("[%s]Retry to write to the client at %s too many times; End connection.\r\n\r\n", timeStr, cliIP);
						tInUse[thPar->thIndex] = 0;	
						tCount--;
						pthread_mutex_unlock(&lockTime);
						pthread_exit(NULL);					
					} */
				}
			}
			
			pthread_mutex_lock(&lockTime);
			getTime();
			fprintf(fp,"[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, thPar->cliSockId, writebuf);
			printf("[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, thPar->cliSockId, writebuf);
			pthread_mutex_unlock(&lockTime);
		}	
	}
	
	//pthread_cleanup_pop(0);
}

//give corresponding behavior to different input
void getResponse(char *readP,char *writeP,int len, tPar *thPar )
{
	int i,check=1,abrt=0;
	BYTE erroStr[40]="ERRO :";
	struSolnP solnP;
	struWorkP workInThP;
	wtPar *wthPar;
	workQP wQP,wQPt;
	
	uint256 targetSoln;
	uint256_init(targetSoln);
	
	//pthread_cleanup_push(cleanupCli, (void *)thPar);
	
	if (len >= 101)
	{
		strcat(erroStr,"message is too loog.\r\n");
		strcpy(writeP,erroStr);
	}
	else if (strcmp(readP,"PING\r\n") ==0)
	{
		strcpy(writeP,"PONG\r\n");
	}
	else if (strcmp(readP,"PONG\r\n") ==0)
	{
		strcat(erroStr,"message PONG is reserved.\r\n");
		strcpy(writeP,erroStr);
	}
	else if (strcmp(readP,"OKAY\r\n") ==0)
	{
		strcat(erroStr,"message OKAY can't be sent.\r\n");
		strcpy(writeP,erroStr);
	}
	else if (strncmp(readP,"ERRO ",5) ==0 || strncmp(readP,"ERRO\r\n",5) ==0)
	{
		strcat(erroStr,"message ERRO is reserved.\r\n");
		strcpy(writeP,erroStr);
	}
	else if (strcmp(readP,"ABRT\r\n") ==0)
	{
		pthread_mutex_lock(&lock);
		
		//clean Queue
		wQP=workQue->next;
		wQPt=workQue;
		while(wQP != NULL)
		{
			if(wQP->cliSockId == thPar->cliSockId)
			{
				pthread_mutex_unlock(&lock);
				pthread_cancel( worktid [wQP->workthId] );
				//pthread_join( worktid [wQP->workthId] ,NULL);
				worktInUse[wQP->workthId]=0;
				wQPt->next=wQP->next;
				wQP->next=NULL;
				//free(wQP);
				wQP = wQPt->next; 
				pthread_mutex_lock(&lock);
				continue;
			}
			wQPt=wQP;
			wQP = wQP->next;
		}	
		
		//clean runing WORK if necessary
		if(workingTh != NULL)
		{
			if(workingTh->cliSockId == thPar->cliSockId)
			{
				pthread_mutex_unlock(&lock);
				pthread_cancel( worktid [workingTh->workthIndex] );
				worktInUse[workingTh->workthIndex]=0;
				workingTh=NULL;
				pthread_mutex_lock(&lock);
			}			
		}		
		
		pthread_mutex_unlock(&lock);
		
		if(abrt == 0)
		{
			strcpy(writeP,"OKAY\r\n");
		}
		else
		{
			strcpy(writeP,"ERRO :can't abort your WORK job.\r\n");
		}
	}
	//SOLN section
	else if (strncmp(readP,"SOLN ",5) ==0)
	{
		solnP=fillSoln(readP);
		if(solnP->diff[28]<0x3 || solnP->diff[28]>0x20)
		{
			strcat(erroStr,"difficulty in SOLN is invalid.\r\n");
			strcpy(writeP,erroStr);
			return;
		}
		getTarget(targetSoln,solnP->diff);
		
		pthread_testcancel();
		check=checkSoln(solnP->x,targetSoln);
		//free(solnP);
		
		if(check == -1)
		{
			strcpy(writeP,"OKAY\r\n");
		}
		else
		{
			strcat(erroStr,"not a valid proof-of-work.\r\n");
			strcpy(writeP,erroStr);
		}
	}
	//WORK section
	else if (strncmp(readP,"WORK ",5) ==0)
	{
		workInThP=fillWork(readP);
		if(workInThP->diff[28]<0x3 || workInThP->diff[28]>0x20)
		{
			strcat(erroStr,"difficulty in WORK is invalid.\r\n");
			strcpy(writeP,erroStr);
			return;
		}
		
		else
		{	
			//for(workthCount=0; workthCount < workInThP->count, workthCount++ )
waitForWorker:		
			//find available worker 
			pthread_testcancel();
			
			pthread_mutex_lock(&lock);
			for(i = 0; i < maxWorker; i ++)
			{
				if(worktInUse[i] == 0)
				{
					worktInUse[i] = 1;
					pthread_mutex_unlock(&lock);
					break;
				}
				else if(i == maxWorker-1)
				{
					pthread_mutex_unlock(&lock);
					goto waitForWorker;
				}
			}
			
			//initialize workthreadParamater
			wthPar=(wtPar *)malloc(sizeof(wtPar));
			wthPar->workthIndex = i; 
			wthPar->cliSockId=thPar->cliSockId;
			memcpy(&(wthPar->cliAddr), &(thPar->cliAddr), sizeof(struct sockaddr_in));
			wthPar->workP=workInThP;

			pthread_create(&(worktid[i]), NULL, workthreadFunc, (void *)wthPar);		
			
		}
	}
	else
	{
		strcat(erroStr,"message has a wrong header.\r\n");
		strcpy(writeP,erroStr);
	}	
	
	//pthread_cleanup_pop(0);
	return;
	
}

//work thread function
void *workthreadFunc(void *workthreadParam) 
{
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	
	//pthread_cleanup_push(cleanupWork, NULL);
	
	int i,n, lenQue=0, flagPended=0;
	char writebuf[maxbuf];
	BYTE *result,*p;
	BYTE buffer[17];
	uint256 target;
	uint256_init(target);
	workQP newQP,outQP;
	wtPar *wthPar = (wtPar *)workthreadParam;
	
	char cliIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(wthPar->cliAddr.sin_addr), cliIP, sizeof(cliIP) );
	
	pthread_mutex_lock(&lock);
	lenQue=lenQ();
	if(lenQue < maxJob)
	{
		newQP=work2Q(wthPar->workP, wthPar->cliSockId, wthPar->workthIndex);	
		inQ(newQP);
		pthread_mutex_unlock(&lock);
	}
	else
	{
		pthread_mutex_unlock(&lock);
		
		bzero(writebuf,sizeof(writebuf));
		pthread_mutex_lock(&lock);
		sprintf(writebuf,"ERRO :10 pending WORK jobs reached.\r\n");
		pthread_mutex_unlock(&lock);
writeagainWork1:		

		pthread_testcancel();
		n = write(wthPar->cliSockId, writebuf, 40);

		pthread_mutex_lock(&lockTime);
		getTime();
		fprintf(fp,"[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, wthPar->cliSockId, writebuf);
		printf("[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, wthPar->cliSockId, writebuf);
		worktInUse[wthPar->workthIndex] = 0;
		pthread_mutex_unlock(&lockTime);
		
		if (n < 0) 
		{
			if (errno == EINTR)
			{
				goto writeagainWork1;
			}
			else
			{
				pthread_mutex_lock(&lockTime);
/* 				getTime();
				fprintf(fp, "[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
				printf("[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP); */
				worktInUse[wthPar->workthIndex] = 0;
				pthread_mutex_unlock(&lockTime);
				pthread_exit(NULL);
			}
		}
		pthread_exit(NULL);	
		
	}
	
	while(1)
	{
		pthread_testcancel();
		
		pthread_mutex_lock(&lock);
		
		if(workingTh ==NULL && workQue->next->workthId == wthPar->workthIndex)
		{
			outQP=outQ();
			workingTh=wthPar;
			
			pthread_mutex_unlock(&lock);
			
			bzero(writebuf,sizeof(writebuf));
			sprintf(writebuf, "Processing WORK for the client at [sock_id:%d].\r\n", wthPar->cliSockId);
			
			pthread_mutex_lock(&lockTime);
			getTime();
			fprintf(fp,"[%s]%s\r\n", timeStr, writebuf);
			printf("[%s]%s\r\n", timeStr, writebuf);
			pthread_mutex_unlock(&lockTime);
			
			/* 
writeagainWork2:		
			pthread_testcancel();
			n = write(wthPar->cliSockId, writebuf, sizeof(writebuf)-1);
			
			if (n < 0) 
			{
				if (errno == EINTR)
				{
					goto writeagainWork2;
				}
				else
				{
					pthread_mutex_lock(&lockTime);
					getTime();
					fprintf(fp, "[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
					printf("[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
					worktInUse[wthPar->workthIndex] = 0;
					workingTh=NULL;
					pthread_mutex_unlock(&lockTime);
					pthread_exit(NULL);
				}
			}
			
			 */
			
			
			getTarget(target, wthPar->workP->diff);
			
			result=getSoln(wthPar->workP,target);
			
			bzero(writebuf,sizeof(writebuf));
			if(result != NULL)	
			{
				memset(buffer, '\0', sizeof(buffer));
				p=buffer;
				for(i=0;i<8;i++)
				{
					sprintf(p,"%02x",result[i]);
					p=p+2;
				}
				strcpy(writebuf, wthPar->workP->strResult);
				strcat(writebuf, " ");
				strncat(writebuf, buffer, 16);
				strcat(writebuf, "\r\n");
			}
			else
			{
				strcpy(writebuf, "ERRO :can't find a solution.\r\n");   			
			}
			
writeagainWork3:	
			pthread_testcancel();
			
			if(strncmp(writebuf,"SOLN",4) ==0 )
			{
				n = write(wthPar->cliSockId, writebuf, 97);
			}
			else
			{
				n = write(wthPar->cliSockId, writebuf, 40);
			}
			
			//n = write(wthPar->cliSockId, writebuf, sizeof(writebuf)-1);
			
			if (n < 0) 
			{
				if (errno == EINTR)
				{
					goto writeagainWork3;
				}
				else
				{
					pthread_mutex_lock(&lockTime);
			/* 		getTime();
					fprintf(fp, "[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
					printf("[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP); */
					worktInUse[wthPar->workthIndex] = 0;
					workingTh=NULL;
					pthread_mutex_unlock(&lockTime);
					pthread_exit(NULL);
				}
			}
			
			pthread_mutex_lock(&lockTime);
			getTime();
			fprintf(fp,"[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, wthPar->cliSockId, writebuf);
			printf("[%s][IP:%s][sock_id:%d]%s\r\n", timeStr, serIP, wthPar->cliSockId, writebuf);			
			worktInUse[wthPar->workthIndex] = 0;
			workingTh=NULL;
			pthread_mutex_unlock(&lockTime);
			pthread_exit(NULL);	
		}
		else
		{		
			pthread_mutex_unlock(&lock);
			
			if(flagPended == 0)
			{
				bzero(writebuf,sizeof(writebuf));
				pthread_mutex_lock(&lock);
				sprintf(writebuf, "Pending WORK for the client at [sock_id:%d]. Now %d pending jobs.\r\n", wthPar->cliSockId, lenQ());
				pthread_mutex_unlock(&lock);
				
				pthread_mutex_lock(&lockTime);
				getTime();
				fprintf(fp,"[%s]%s\r\n", timeStr, writebuf);
				printf("[%s]%s\r\n", timeStr, writebuf);
				pthread_mutex_unlock(&lockTime);
				
				flagPended=1;
				
				/* 
writeagainWork4:	
				pthread_testcancel();
				n = write(wthPar->cliSockId, writebuf, sizeof(writebuf)-1);
				
				if (n < 0) 
				{
					if (errno == EINTR)
					{
						goto writeagainWork4;
					}
					else
					{
						pthread_mutex_lock(&lockTime);
						getTime();
						fprintf(fp, "[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
						printf("[%s]ERROR write to the client at %s.\r\n\r\n", timeStr, cliIP);
						worktInUse[wthPar->workthIndex] = 0;
						pthread_mutex_unlock(&lockTime);
						pthread_exit(NULL);
					}
				}*/				
				
				
			} 
		}
	}
	
	//pthread_cleanup_pop(0);
}

//need to lock when use
void getTime()
{
	memset(timeStr, '\0', sizeof(timeStr));
	time(&timer);
	timeInfo = localtime(&timer);
	strftime(timeStr, 30, "%d/%m/%Y %H:%M:%S", timeInfo);
}

workQP work2Q(struWorkP incomeWorkP,int incomeCliSockId,int incomeWorkthId)
{
	workQP new=(workQP)malloc(sizeof(workQE));
	new->workP=incomeWorkP;
	new->cliSockId=incomeCliSockId;
	new->workthId=incomeWorkthId;
	new->next=NULL;
	return new;
}

//need to lock when use
workQP outQ()
{
	workQP out=workQue->next;
	if(out != NULL)
	{
		workQue->next=out->next;
		out->next=NULL;
	}
	return out;
}

//need to lock when use
void inQ(workQP newWork)
{
	workQP in=workQue;
	while(in->next != NULL)
	{
		in=in->next;
	}
	in->next=newWork;
}

//need to lock when use
int lenQ()
{
	int len=0;
	workQP temp=workQue;
	while(temp->next != NULL)
	{
		len++;
		temp=temp->next;
	}	
	return len;
}


//thread cleanup handler
//unlock all locks; trylock first in case it have not locked.
/* void cleanupCli(void *cleanCliParam)
{   
	char buffer[maxbuf];
	workQP wQP,wQPt;
	
	memset(buffer, '\0', sizeof(buffer));
	
	char cliIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(( (tPar *)cleanCliParam )->cliAddr.sin_addr), cliIP, sizeof(cliIP) );
	
	pthread_mutex_trylock(&lock);

	//clean Queue
	wQP=workQue->next;
	wQPt=workQue;
	while(wQP != NULL)
	{		
		if(wQP->cliSockId == ( (tPar *)cleanCliParam )->cliSockId)
		{
			pthread_cancel( worktid [wQP->workthId] );
			worktInUse [wQP->workthId]=0;
			wQPt->next=wQP->next;
			wQP->next=NULL;
			//free(wQP);
			wQP = wQPt->next;
			continue;
		}			
		wQPt=wQP;
		wQP = wQP->next;
	}
	
	//clean runing WORK if necessary
	if( workingTh !=NULL)  
	{		
		if( workingTh->cliSockId == ( (tPar *)cleanCliParam )->cliSockId ) 
		{
			pthread_cancel( worktid [workingTh->workthIndex] );
			worktInUse [workingTh->workthIndex]=0;
			//free(workingTh);
			workingTh=NULL;
		}		
	}
	
	getTime();
	sprintf(buffer,"[%s]all threads of the client at %s has been closed.\r\n\r\n", timeStr, cliIP);
	fprintf(fp,"%s", buffer);
	printf("%s", buffer);
	
	tInUse[( (tPar *)cleanCliParam )->thIndex] = 0;
	close(( (tPar *)cleanCliParam )->cliSockId);
	free(  ( (tPar *)cleanCliParam ) );
	
	pthread_mutex_unlock(&lock);
	pthread_mutex_trylock(&lockTime);
	pthread_mutex_unlock(&lockTime);
}


void cleanupWork(void *arg)
{   
	workQP wQP,wQPt;
	
	printf("true\n");
	pthread_mutex_trylock(&lock);
	worktInUse[( (wtPar *)cleanWorkParam )->workthIndex] = 0;
	
	//clean Queue
	wQP=workQue->next;
	wQPt=workQue;
	while(wQP != NULL)
	{
		printf("l=%d",lenQ());
		if(wQP->workthId == ( (wtPar *)cleanWorkParam )->workthIndex)
		{
			
			pthread_cancel( worktid [wQP->workthId] );
			worktInUse [wQP->workthId] =0;
			wQPt->next=wQP->next;
			wQP->next=NULL;
			//free(wQP);
			wQP = wQPt->next;
			printf("len=%d",lenQ());
			break;
		}			
		wQPt=wQP;
		wQP = wQP->next;
	}
	
	//clean runing WORK if necessary
	if(workingTh != NULL ) 
	{
		if( ( (wtPar *)cleanWorkParam )->workthIndex == workingTh->workthIndex )
		{
			pthread_cancel( worktid [workingTh->workthIndex] );
			worktInUse [workingTh->workthIndex] =0;
			//free(workingTh);
			workingTh=NULL;
			printf("work\n");
		}			

	
	pthread_mutex_trylock(&lock);
	pthread_mutex_unlock(&lock);
	pthread_mutex_trylock(&lockTime);
	pthread_mutex_unlock(&lockTime);
} */


