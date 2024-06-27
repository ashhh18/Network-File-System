#ifndef DEFS
#define DEFS

#include "HashTable.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

#define IP "127.0.0.1"
#define NS_MASTER_PORT 10000
#define MAX_CONNECTIONS 10
#define MAXSIZES 1024
#define LRU_MAX 10

#define RESET "\033[0m"
#define ORANGE "\e[38;2;255;85;0m"
#define RED "\033[1;31m"
#define CYAN "\033[1;36m"
#define YELLOW "\033[1;33m"
#define WHITE "\033[1;37m"
#define BLUE "\033[1;34m"
#define GREEN "\033[1;32m"

#define SS_INIT 100
#define SS_DEFPATH 101
#define PATHNOTFOUND 303
#define RWSERROR 402
#define SUCCESS 300
#define BADFILEPATH 401
#define EXPPATHDIR 405
#define FAILURE 299
#define TUSRCHAI 69
#define TUDESTHAI 79
#define SERVERDISCONN 80
#define COPYFLAG 81
typedef enum destinationFlags
{
    NtoS,
    StoN,
    CtoN,
    NtoC,
    CtoS,
    StoC,
    STOP,
} destinationFlags;

typedef enum dataFlags
{
    readReq,
    writeReq,
    deltReq,
    LSreq,
    createReq,
    cpyReq,
    redData,
    getSP,
    conditionCode,
    portbhejrhahoon,
    imdying,
    logreq,
} dataFlags;

typedef struct dataStruct
{
    destinationFlags destF;
    dataFlags datF;
    int intData[7];
    char stringData[5][MAXSIZES + 1];
} dataStruct;

typedef struct MainArgs
{
    dataStruct *Main;
    int portForCli;
    int newPortForSS;
    int oldSSPort;
    int flag;
    int srcPortold;
    int destPortold;
    int srcPortnew;
    int destPortnew;
    int destIdx;
    char homepathSrc[MAXSIZES];
    char homepathDest[MAXSIZES];
} MainArgs;

typedef struct LogStruct
{
    dataFlags datF;
    int port_used_by_client;
    int port_used_by_serverSrc;
    int port_used_by_serverDest;
    int SS_idxsrc;
    int SS_idxdest;
    int flag; // 0 for non priority and 1 for priority
} LogStruct;

typedef LogStruct *Log;
void *ListenOnAPort(void *arg);

#endif
