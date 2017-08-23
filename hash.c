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

/*  int main(int argc, char **argv)
{
	BYTE *result;

	struSolnP solnP;
	struWorkP workP;
	// char buffer1[] = "SOLN 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212147\n";
	char buffer[] = "WORK 1fffffff 0000000019d6689c085ae165831e934ff763ae46a218a6c172b3f1b60a8ce26f 1000000023212140 01\n";
	
	// solnP=fillSoln(buffer1);
	workP=fillWork(buffer);
	
	// uint256 targetSoln;
	// uint256_init(targetSoln);
	// getTarget(targetSoln,solnP->diff);

	// int i=checkSoln(solnP->x,targetSoln);

	uint256 targetWork;
	uint256_init(targetWork);
	getTarget(targetWork, workP->diff);

	result=getSoln(workP, targetWork);
	printf("0x");
	for (size_t i = 0; i < 8; i++) {
		printf("%02x", result[i]);
	}
	printf("\n");
	printf("%s\n",workP->strResult);

	return 0;
}  */

int checkSoln(BYTE *x, uint256 target)
{
	int i;
	SHA256_CTX ctx;
	uint256 hx,y;
	uint256_init(hx);
	uint256_init(y);
	
	sha256_init(&ctx);
	sha256_update(&ctx, x, 40);
	sha256_final(&ctx,hx);
	
	sha256_init(&ctx);
	sha256_update(&ctx, hx, 32);
	sha256_final(&ctx, y);
	// print_uint256(y);

	i=sha256_compare(y,target);
	
	return i;
}

//return NULL when no result
BYTE *getSoln(struWorkP workP, uint256 target)
{
	BYTE *result = (BYTE *)malloc(8 * sizeof(BYTE));
	memset(result, '\0', 8);

	int check=1;
	BYTE x[41];
	memset(x, '\0', sizeof(x));
	uint256 adder,one;
	uint256_init(adder);
	uint256_init(one);
	one[31] = 0x1;

	//pad workP->start to uint256
	for (int i = 0; i < 8; i++)
	{
		adder[i + 24] = workP->start[i];
	}
	//construct x and find result
	memcpy(x, workP->seed, 32);
	while (adder[23] == 0x0)
	{
		//pthread_testcancel();
		memcpy(&x[32],&adder[24], 8);
		check = checkSoln(x, target);
		if (check == -1)
		{
			memcpy(result, &adder[24], 8);
			return result;
		}
		uint256_add(adder, adder, one);
	}
	free(result);
	return NULL;
}

void getTarget(uint256 target, uint256 diff)
{
	int i;
	uint256 alpha, beta;
	//uint256 buf1, buf2;
	//uint32_t exp;
	uint256_init(alpha);
	uint256_init(beta);
	//uint256_init(buf1);
	//uint256_init(buf2);

	//fill alpha, beta
	alpha[31]=diff[28];
	for(i=0;i<3;i++)
	{
		beta[i+29]=diff[i+29];
	}
	// exp=(uint32_t)0x8*(uint32_t)(alpha[31]-3);   //8*(alpha-3)
	// buf2[31]=0x2;
	// uint256_exp( buf1, buf2, exp);		//2exp( 8*(alpha-3) )
	// uint256_mul( target, beta, buf1);	//beta* 2exp( 8*(alpha-3) )
	// print_uint256(target);
	
	//beta* 2exp( 8*(alpha-3) )
	BYTE shift=(BYTE)0x8*(BYTE)(alpha[31]-(BYTE)3);
	uint256_sl( target, beta, shift);
	// print_uint256(target);	
}

struSolnP fillSoln(char *buffer)
{
	int i;
	BYTE *p,temp[3], bufx[81];
	memset(temp, '\0', sizeof(temp));
	struSolnP solnP=(struSolnP)malloc(sizeof(struSoln));
	uint256_init(solnP->diff);
	memset(solnP->x, '\0', sizeof(solnP->x));
	
	BYTE strPtr[5],strDiff[9],strSeed[65],strSolu[17];
	// printf("%s", buffer);
	sscanf( buffer, "%s %s %s %s\r\n", strPtr, strDiff, strSeed, strSolu);
	// printf("%s %s %s %s\n", strPtr, strDiff, strSeed, strSolu);
	
	//fill difficulty
	p=strDiff;
	i=0;
	while(*p!='\0')
	{
		strncpy(temp,p,2);
		sscanf(temp,"%hhx",&solnP->diff[28+i]);
		p=p+2;
		i++;
	}
	// print_uint256(solnP->diff);
	
	//fill x
	strcpy(bufx,strSeed);
	strcat(bufx,strSolu);
	// printf("%s", bufx);
	// printf("\n");

	p = bufx;
	i = 0;
	while (*p != '\0')
	{
		strncpy(temp, p, 2);
		sscanf(temp, "%hhx", &solnP->x[i]);
		// printf("%02x", solnP->x[i]);
		p = p + 2;
		i++;
	}
	// printf("\n");

	return solnP;
}

struWorkP fillWork(char *buffer)
{
	int i;
	BYTE *p,temp[3];
	memset(temp, '\0', sizeof(temp));
	struWorkP workP=(struWorkP)malloc(sizeof(struWork));
	memset(workP->strResult, '\0', sizeof(workP->strResult));
	uint256_init(workP->diff);
	
	BYTE strPtr[5],strDiff[9],strSeed[65],strStart[17],strCount[3];
	// printf("%s", buffer);
	sscanf( buffer, "%s %s %s %s %s\r\n", strPtr, strDiff, strSeed, strStart,strCount);
	// printf("%s %s %s %s %s\n", strPtr, strDiff, strSeed, strStart,strCount);
	
	//fill strResult
	strcpy(workP->strResult,"SOLN ");
	strncat(workP->strResult,strDiff,8);
	strcat(workP->strResult," ");
	strncat(workP->strResult,strSeed,64);
	//printf("%s",workP->strResult);
	
	//fill difficulty
	p=strDiff;
	i=0;
	while(*p!='\0')
	{
		strncpy(temp,p,2);
		sscanf(temp,"%hhx",&workP->diff[28+i]);
		p=p+2;
		i++;
	}
	// print_uint256(workP->diff);

	//fill seed
	p=strSeed;
	i=0;
	while(*p!='\0')
	{
		strncpy(temp,p,2);
		sscanf(temp,"%hhx",&workP->seed[i]);
		// printf("%02x",workP->seed[i]);
		p=p+2;
		i++;
	}
	// printf("\n");
	
	//fill start
	p=strStart;
	i=0;
	// printf("0x");
	while(*p!='\0')
	{
		strncpy(temp,p,2);
		sscanf(temp,"%hhx",&workP->start[i]);
		// printf("%02x",workP->start[i]);
		p=p+2;
		i++;
	}
	// printf("\n");
	
	//fill count
	p=strCount;
	i=0;
	// printf("0x");
	while(*p!='\0')
	{
		strncpy(temp,p,2);
		sscanf(temp,"%hhx",&workP->count);
		// printf("%02x",workP->count);
		p=p+2;
		i++;
	}
	// printf("\n");
	
	return workP;
}



