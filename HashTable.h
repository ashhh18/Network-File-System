#ifndef HASHTABLE
#define HASHTABLE

#include "defs.h"

#define MAXLENGTH 131072
#define capacity 131072
#define PRIME 31

typedef struct node
{
    int val;
    char *s;
    struct node *next;
} node;
typedef node *Node;

typedef struct hashTable
{
    Node arr[MAXLENGTH];
} hashTable;
typedef hashTable *HT;

HT initTable();
int hashingFunc(char *s);
void insertNode(Node N, int e, char *s);
void insert(int e, char *s, HT h);
int searchPath(char *s, HT h);
int deletePath(char *s, HT h);

#endif