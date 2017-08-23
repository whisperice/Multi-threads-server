// COMP30023 Computer Systems Project 2
// Name: Yitao Deng
// User ID: yitaod
// Student ID: 711645


#define maxCli 101
#define maxJob 10
#define maxWorker 1000
#define maxbuf 256
#define serIP "0.0.0.0"

//type definition

//typedef unsigned char BYTE;
typedef BYTE uint256[32];

typedef struct structSoln
{
	uint256 diff;
	BYTE x[41];
}struSoln, *struSolnP;

typedef struct structWork
{
	uint256 diff; 
	BYTE seed[32],start[8];
	BYTE count;
	BYTE strResult[79];
}struWork, *struWorkP;

typedef struct structWorkQ
{
	struWorkP workP;
	int cliSockId, workthId;
	struct structWorkQ *next;
}workQE, *workQP;

typedef struct threadParamater
{
	int cliSockId,thIndex;
	struct sockaddr_in cliAddr;
}tPar;

typedef struct workthreadParamater
{
	int cliSockId,workthIndex;
	struct sockaddr_in cliAddr;
	struWorkP workP;
}wtPar;


//functions definition
void *threadFunc(void *);
void getResponse(char *,char *,int, tPar *);
void *workthreadFunc(void *);
void getTime();
void cleanupCli(void *);
void cleanupWork(void *);

workQP work2Q(struWorkP, int,int);
workQP outQ();
void inQ(workQP);
int lenQ();

struSolnP fillSoln( char * );
struWorkP fillWork( char * );
void getTarget(uint256, uint256);
int checkSoln(uint256, uint256);
BYTE *getSoln(struWorkP, uint256);