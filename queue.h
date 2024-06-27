#include "defs.h"
#define MAXSIZES 1024

struct element 
{
    int quidx;
    char cached_path[MAXSIZES];
};
typedef struct element *Element;

struct qnode
{
    struct element *s;
    struct qnode *next;
};
typedef struct qnode *QNode;

struct que
{
    QNode front;
    QNode rear;
    int maxSize;
    int size;
};
typedef struct que *Queue;

int enqueue (Queue Q,QNode E);
QNode removeMiddle (Queue Q, int idx);
QNode dequeue (Queue Q);
void init (Queue Q, int maxx);
int isEmpty (Queue Q);
int isFull (Queue Q);
int LRU (char* path, Queue q);
